"""Offline converter for GTAVCS.PPSSPP.GamepadIcons.

Turns the PPSSPP replacement art in textures/GTAVCS/replacements/buttons/<set>
into the game's native rasters (same size, 4-bit, RGBA8888 palette, GE
swizzle, mip levels) and stores, per set, only the bytes that differ from the
game's original texture. The plugin keeps the original it loaded and applies
these runs on top, so no game artwork is redistributed.

The original rasters (raster header + bytes, as the game loads them) come from
a running game: --originals points at a folder with <name>.raw files and an
info.json describing them (captured with the plugin's Debug log).

usage: python tools/gamepad-icons/convert.py --originals DIR
"""
import argparse, json, os, struct, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SETS = [('360', 'xbox360'), ('XBO', 'xboxone'), ('PS3', 'ps3'), ('PS4', 'ps4'), ('PS5', 'ps5'), ('SWI', 'switch'), ('DEK', 'steamdeck')]
FILES = {'GameText': 'gametext', 'fe_controller_new': 'fe_controller_new', 'fe_arrows_onfoot_layout1': 'fe_arrows_onfoot_layout1',
         'fe_arrows_invehicle_layout2': 'fe_arrows_invehicle_layout2', 'fe_arrows_layout_right': 'fe_arrows_layout_right'}


def layout(w, h, stride, levels):
    out, off = [], 0
    for _ in range(levels):
        out.append((off, w, h, stride))
        off += stride * h
        if stride >= 17: stride //= 2
        w, h = max(1, w // 2), max(1, h // 2)
    return out, off


def swz(x_byte, y, stride):
    return ((y // 8) * (stride // 16) + x_byte // 16) * 128 + (y % 8) * 16 + x_byte % 16


def decode(raw, info):
    levels = info['levels'] & 63
    lay, pal_off = layout(info['w'], info['h'], info['stride'], levels)
    pal = np.frombuffer(raw[pal_off:pal_off + 64], np.uint8).reshape(16, 4).copy()
    out = []
    for off, w, h, stride in lay:
        idx = np.zeros((h, w), np.uint8)
        for y in range(h):
            for x in range(w):
                b = raw[off + swz(x // 2, y, stride)]
                idx[y, x] = b >> 4 if x & 1 else b & 15
        out.append(idx)
    return out, pal, lay, pal_off


def encode(levels_idx, pal, lay, pal_off, size):
    raw = bytearray(size)
    for idx, (off, w, h, stride) in zip(levels_idx, lay):
        for y in range(h):
            for x in range(w):
                o = off + swz(x // 2, y, stride)
                if x & 1: raw[o] = (raw[o] & 0x0F) | (int(idx[y, x]) << 4)
                else: raw[o] = (raw[o] & 0xF0) | int(idx[y, x])
    raw[pal_off:pal_off + 64] = pal.astype(np.uint8).tobytes()
    return bytes(raw)


def resize(img, h, w):
    """Box filter in premultiplied alpha down to (h, w)."""
    img = img.astype(np.float64)
    if img.shape[0] == h and img.shape[1] == w: return img
    ky, kx = img.shape[0] // h, img.shape[1] // w
    a = img[..., 3:4] / 255.0
    pre = np.concatenate([img[..., :3] * a, img[..., 3:4]], -1)
    pre = pre[:h * ky, :w * kx].reshape(h, ky, w, kx, 4).mean((1, 3))
    a = pre[..., 3:4] / 255.0
    rgb = np.where(a > 0, pre[..., :3] / np.maximum(a, 1e-6), 0)
    return np.concatenate([rgb, pre[..., 3:4]], -1)


def distance(pixels, pal):
    """Premultiplied RGBA distance (N x K)."""
    pa = pixels[:, None, 3:4] / 255.0
    qa = pal[None, :, 3:4] / 255.0
    d = (pixels[:, None, :3] * pa - pal[None, :, :3] * qa) ** 2
    return d.sum(-1) + 1.5 * (pixels[:, None, 3] - pal[None, :, 3]) ** 2


def quantize(pixels, pal, free, iterations=12):
    """k-means over `pixels` where only the entries in `free` may move."""
    pal = pal.astype(np.float64).copy()
    if len(pixels) == 0: return pal
    for _ in range(iterations):
        nearest = distance(pixels, pal).argmin(1)
        for k in free:
            sel = pixels[nearest == k]
            if len(sel):
                a = sel[:, 3:4] / 255.0
                rgb = (sel[:, :3] * a).sum(0) / max(a.sum(), 1e-6)
                pal[k] = [*rgb, sel[:, 3].mean()]
            else:
                # Seed an unused entry with the worst-fitting pixel.
                worst = distance(pixels, pal).min(1).argmax()
                pal[k] = pixels[worst]
    return np.clip(np.round(pal), 0, 255)


def convert(name, info, raw, art, reference):
    """Native raster bytes of `art`; `reference` (the PSP replacement) marks which texels changed for the font."""
    idxs, pal, lay, pal_off = decode(raw, info)
    h, w = info['h'], info['w']
    target = resize(art, h, w)
    if name == 'GameText':
        # Only the button glyphs differ between sets: keep every other texel of the game's font.
        ref = resize(reference, h, w)
        changed = (np.abs(target - ref).sum(-1) > 48)
        grow = changed.copy()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                grow |= np.roll(np.roll(changed, dy, 0), dx, 1)
        changed = grow
    else:
        changed = np.ones((h, w), bool)
    keep = set(np.unique(idxs[0][~changed]).tolist())
    for l in range(1, len(idxs)):
        cy, cx = changed.shape[0] // idxs[l].shape[0], changed.shape[1] // idxs[l].shape[1]
        small = changed[:idxs[l].shape[0] * cy, :idxs[l].shape[1] * cx].reshape(idxs[l].shape[0], cy, idxs[l].shape[1], cx).any((1, 3))
        keep |= set(np.unique(idxs[l][~small]).tolist())
    free = [k for k in range(16) if k not in keep]
    pixels = target[changed].reshape(-1, 4)
    newpal = quantize(pixels, pal, free)
    out = [idxs[0].copy()]
    out[0][changed] = distance(pixels, newpal).argmin(1)
    level_img = target
    for l in range(1, len(idxs)):
        lh, lw = idxs[l].shape
        level_img = resize(level_img, lh, lw)
        cy, cx = changed.shape[0] // lh, changed.shape[1] // lw
        small = changed[:lh * cy, :lw * cx].reshape(lh, cy, lw, cx).any((1, 3))
        cur = idxs[l].copy()
        cur[small] = distance(level_img[small].reshape(-1, 4), newpal).argmin(1)
        out.append(cur)
    return encode(out, newpal, lay, pal_off, info['bytes']), newpal, out[0]


def runs(original, new, gap=8):
    """Byte ranges where `new` differs from `original`, merging close ranges."""
    diff = np.frombuffer(original, np.uint8) != np.frombuffer(new, np.uint8)
    out, i, n = [], 0, len(diff)
    while i < n:
        if not diff[i]: i += 1; continue
        j = i
        while j < n and (diff[j] or diff[j:j + gap].any()): j += 1
        out.append((i, new[i:j]))
        i = j
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--originals', required=True)
    ap.add_argument('--out', default=os.path.join(ROOT, 'data', 'GTAVCS.PPSSPP.GamepadIcons', 'memstick', 'PSP', 'PLUGINS',
                                                  'GTAVCS.PPSSPP.GamepadIcons', 'icons'))
    ap.add_argument('--preview', default=None)
    a = ap.parse_args()
    infos = json.load(open(os.path.join(a.originals, 'info.json')))
    art_root = os.path.join(ROOT, 'textures', 'GTAVCS', 'replacements', 'buttons')
    os.makedirs(a.out, exist_ok=True)
    previews = []
    for code, folder in SETS:
        blob = bytearray()
        row = []
        for name, info in infos.items():
            file = FILES.get(name, name.replace('_PSP', '_psp'))
            art_path = os.path.join(art_root, folder, file + '.png')
            if not os.path.exists(art_path): continue
            raw = open(os.path.join(a.originals, name + '.raw'), 'rb').read()
            art = png.read(art_path)
            ref = png.read(os.path.join(art_root, 'psp', file + '.png'))
            new, pal, idx0 = convert(name, info, raw, art, ref)
            rs = runs(raw, new)
            entry = name.encode().ljust(32, b'\0')
            entry += struct.pack('<BBBBhHII', info['w'].bit_length() - 1, info['h'].bit_length() - 1, info['depth'],
                                 info['levels'], info['stride'], info['flags'], info['bytes'], len(rs))
            for off, data in rs:
                entry += struct.pack('<II', off, len(data)) + data
                entry += b'\0' * (-len(entry) % 4)
            blob += entry
            row.append(pal[idx0])
        open(os.path.join(a.out, code + '.bin'), 'wb').write(blob)
        print(code, len(blob), 'bytes')
        if a.preview:
            tiles = []
            for im in row:
                t = np.zeros((256, 256, 4)); t[:im.shape[0], :im.shape[1]] = im; tiles.append(t)
            previews.append(np.concatenate(tiles, 1))
    if a.preview:
        img = np.concatenate(previews, 0)
        a_ = img[..., 3:4] / 255.0
        png.write(a.preview, (img[..., :3] * a_ + 60 * (1 - a_)).astype(np.uint8))


if __name__ == '__main__':
    main()
