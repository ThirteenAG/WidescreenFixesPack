"""Offline converter for the controller icons of GTACTW.PPSSPP.FusionFix.

Chinatown Wars draws button prompts from its icon font, one 256x256 4-bit
texture. The plugin binds an 8-bit copy of that texture instead: the game's own
texels and 16 colours come from the loaded texture, and each set file supplies
only the replaced button cells and the colours they use, drawn from the PPSSPP
replacement art in textures/GTAVCS/replacements/buttons/<set>.

Set file (little endian): 'CTWI', u16 version, u16 colours, u16 cells, u16 0,
colours * RGBA8888 (CLUT entries 16..), then per cell u8 x, y, w, h and w*h
CLUT indices (16..255).

usage: python tools/gamepad-icons/ctw.py [--preview out.png]
"""
import argparse, os, struct, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png
from convert import resize, quantize, distance

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
ART = os.path.join(ROOT, 'textures', 'GTAVCS', 'replacements', 'buttons')
OUT = os.path.join(ROOT, 'data', 'GTACTW.PPSSPP.FusionFix', 'memstick', 'PSP', 'PLUGINS', 'GTACTW.PPSSPP.FusionFix', 'icons')
SETS = [('360', 'xbox360'), ('XBO', 'xboxone'), ('PS3', 'ps3'), ('PS4', 'ps4'), ('PS5', 'ps5'), ('SWI', 'switch'), ('DEK', 'steamdeck')]
# Icon font cells (glyphs 0xFF10..): x, y, art file, art box (left, top, width, height) inside the 18x16 cell.
FACE = (2, 0, 12, 12)
DPAD = (3, 0, 12, 13)
CELLS = [
    (0, 0, 'btn_circle_psp', FACE),
    (18, 0, 'btn_cross_psp', FACE),
    (36, 0, 'btn_triangle_PSP', FACE),
    (54, 0, 'btn_square_psp', FACE),
    (105, 32, 'btn_up_psp', DPAD),
    (123, 32, 'btn_down_psp', DPAD),
    (141, 32, 'btn_left_psp', DPAD),
    (159, 32, 'btn_right_psp', DPAD),
]
W, H = 18, 16
FIRST = 16          # CLUT entries 0..15 stay the game's colours
TRANSPARENT = FIRST  # entry 16 is fully transparent
COLOURS = 256 - FIRST


def fit(art, box):
    """Crop the art to its visible pixels and scale it into the box, centred, keeping its proportions."""
    left, top, bw, bh = box
    alpha = art[..., 3] > 8
    ys, xs = np.nonzero(alpha)
    art = art[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    h, w = art.shape[:2]
    scale = min(bw / w, bh / h)
    tw, th = max(1, round(w * scale)), max(1, round(h * scale))
    # Box filter needs an integer factor: upsample by repetition first.
    ky, kx = -(-th * 8 // h), -(-tw * 8 // w)
    big = np.repeat(np.repeat(art, ky, 0), kx, 1)
    small = resize(big[:th * (big.shape[0] // th), :tw * (big.shape[1] // tw)], th, tw)
    cell = np.zeros((H, W, 4))
    x, y = left + (bw - tw) // 2, top + (bh - th) // 2
    cell[y:y + th, x:x + tw] = small
    return cell


def build(folder):
    cells = [fit(png.read(os.path.join(ART, folder, name + '.png')).astype(np.float64), box) for _, _, name, box in CELLS]
    pixels = np.concatenate([c.reshape(-1, 4) for c in cells])
    visible = pixels[pixels[:, 3] >= 8]
    unique = np.unique(np.round(visible), axis=0)
    if len(unique) <= COLOURS - 1:
        pal = unique
    else:
        seed = unique[np.linspace(0, len(unique) - 1, COLOURS - 1).astype(int)]
        pal = quantize(visible, seed, list(range(COLOURS - 1)))
    pal = np.clip(np.round(pal), 0, 255)
    out = []
    for (x, y, _, _), cell in zip(CELLS, cells):
        flat = cell.reshape(-1, 4)
        idx = distance(flat, pal).argmin(1) + TRANSPARENT + 1
        idx[flat[:, 3] < 8] = TRANSPARENT
        out.append((x, y, idx.astype(np.uint8)))
    return pal.astype(np.uint8), out


def main():
    a = argparse.ArgumentParser()
    a.add_argument('--preview')
    o = a.parse_args()
    os.makedirs(OUT, exist_ok=True)
    rows = []
    for code, folder in SETS:
        pal, cells = build(folder)
        colours = np.concatenate([np.zeros((1, 4), np.uint8), pal])
        blob = b'CTWI' + struct.pack('<HHHH', 1, len(colours), len(cells), 0) + colours.tobytes()
        for x, y, idx in cells:
            blob += struct.pack('<BBBB', x, y, W, H) + idx.tobytes()
        open(os.path.join(OUT, code + '.bin'), 'wb').write(blob)
        print(code, len(colours), 'colours', len(blob), 'bytes')
        if o.preview:
            lut = np.zeros((256, 4), np.uint8); lut[FIRST:FIRST + len(colours)] = colours
            rows.append(np.concatenate([lut[idx].reshape(H, W, 4) for _, _, idx in cells], 1))
    if o.preview:
        img = np.concatenate(rows, 0).astype(np.float64)
        img = np.repeat(np.repeat(img, 4, 0), 4, 1)
        alpha = img[..., 3:4] / 255.0
        png.write(o.preview, (img[..., :3] * alpha + 60 * (1 - alpha)).astype(np.uint8))


if __name__ == '__main__':
    main()
