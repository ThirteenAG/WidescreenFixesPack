"""Converts the hand-painted ImVehLM replacement art (textures/GTAVCS, textures/GTALCS)
into paletted textures the plugins copy into their own variant rasters.

Two packs per plugin:
  native: the game texture's own size, depth (VCS 4-bit, LCS 8-bit) and mip
          count; used when the HD pool is full.
  hd:     the art's full resolution (up to 256x256), 8-bit / 256 colours, the
          same mip count as the native raster (PSP: down to 2x2); the plugins
          give their HD rasters this size and depth.
Every one of the eight states (headlights off/on x tail lights off/on/brake/
reverse, the "off" state included) gets its own palette but shares the "off"
state's indices wherever its texels did not change, so the other seven are
stored as small differences. Mip levels are made from the full-size art.

Block (little endian): u8 widthLog2, heightLog2, depth, levels; u16 colours,
variants (8); u32 texels; base indices (levels concatenated, row major, 4-bit low
nibble first); palette (colours x RGBA, native alpha); then for variants 1..7:
u16 changed entries {u8 index, RGBA}, u32 run bytes, runs {u16 skip, u16 count,
u8 index[count]}.
Pack: "IVL2", u32 count, u32 offset[count] (0 = no art); at each offset u32 raw
size, u32 stored size, the block compressed with LZ4 (lz.py).

Output:
  PSP: data/<plugin>/memstick/PSP/PLUGINS/<plugin>/lights.bin and lights-hd.bin
  PS2: source/<plugin>/LightPack.hpp (both packs embedded; the PCSX2 plugin host has no file access)

Run manually (textures/GTAVCS/build-lights.bat, textures/GTALCS/build-lights.bat);
the normal build never runs it, the generated files are committed.
usage: python tools/vehicle-lights/pack.py [--game GTAVCS|GTALCS] [--only PLUGIN[,PLUGIN]] [--preview DIR]
"""
import argparse, glob, os, struct, sys, time
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png, lz
from quant import resize, plan_variants
from generate import VARIANTS, models, ROOT

# plugin, game, native depth, native mip levels, HD mip levels, opaque alpha, output
PLUGINS = [
    ('GTAVCS.PCSX2F.ImVehLM', 'GTAVCS', 4, 1, 1, 128, 'header'),
    ('GTAVCS.PPSSPP.ImVehLM', 'GTAVCS', 4, 8, 8, 255, 'file'),
    ('GTALCS.PPSSPP.ImVehLM', 'GTALCS', 8, 8, 8, 255, 'file'),
    ('GTALCS.PCSX2F.ImVehLM', 'GTALCS', 8, 5, 5, 128, 'header'),
]
HD_LIMIT = 256


def native_size(name, art, dumps):
    if name in dumps:
        return tuple(png.read(dumps[name]).shape[:2])
    h, w = art.shape[:2]
    return (h // 2, w // 2) if max(h, w) >= 256 else (h, w)


def plan(art_dir, size, depth, max_levels):
    """Quantised states for (h, w) with up to max_levels mips (down to 1x1)."""
    h, w = size
    full = [png.read(os.path.join(art_dir, v + '.png')).astype(np.float64) for v in VARIANTS]
    levels, lw, lh = [], w, h
    while len(levels) < max_levels:
        levels.append((lw, lh))
        if lw == 1 and lh == 1:
            break
        lw, lh = max(1, lw // 2), max(1, lh // 2)
    per_variant = [[resize(f, lh, lw) for lw, lh in levels] for f in full]
    pals, idxs = plan_variants(per_variant, 1 << depth)
    return dict(w=w, h=h, depth=depth, levels=levels, pals=pals, idxs=idxs)


def encode(p, levels, alpha):
    w, h, depth = p['w'], p['h'], p['depth']
    levels = min(levels, len(p['levels']))
    texels = sum(lw * lh for lw, lh in p['levels'][:levels])
    colours = 1 << depth
    idxs = [i[:texels] for i in p['idxs']]

    def native(pal):
        pal = np.clip(np.round(pal), 0, 255).astype(np.int64)
        if alpha != 255:
            pal[:, 3] = (pal[:, 3] * alpha + 127) // 255
        return pal.astype(np.uint8)

    out = bytearray(struct.pack('<BBBBHHI', w.bit_length() - 1, h.bit_length() - 1, depth, levels, colours, 8, texels))
    base = idxs[0].astype(np.uint8)
    if depth == 4:
        b = base if len(base) % 2 == 0 else np.append(base, 0)
        out += (b[0::2] | (b[1::2] << 4)).astype(np.uint8).tobytes()
    else:
        out += base.tobytes()
    pal0 = native(p['pals'][0])
    out += pal0.tobytes()
    for v in range(1, 8):
        palv = native(p['pals'][v])
        changes = [i for i in range(colours) if (palv[i] != pal0[i]).any()]
        out += struct.pack('<H', len(changes))
        for i in changes:
            out += bytes([i]) + palv[i].tobytes()
        diff = np.nonzero(idxs[v] != idxs[0])[0]
        runs = bytearray()
        pos, i = 0, 0
        while i < len(diff):
            start = int(diff[i])
            j = i
            while j + 1 < len(diff) and diff[j + 1] - diff[j] <= 4 and diff[j + 1] - start < 65535:
                j += 1
            end = int(diff[j]) + 1
            while start - pos > 65535:
                runs += struct.pack('<HH', 65535, 0)
                pos += 65535
            runs += struct.pack('<HH', start - pos, end - start) + idxs[v][start:end].astype(np.uint8).tobytes()
            pos = end
            i = j + 1
        out += struct.pack('<I', len(runs)) + runs
    return bytes(out)


def pack_blob(blocks):
    count = len(blocks)
    table = bytearray(b'IVL2' + struct.pack('<I', count))
    body = bytearray()
    offset = 8 + 4 * count
    for data in blocks:
        if not data:
            table += struct.pack('<I', 0)
            continue
        packed = lz.compress(data)
        table += struct.pack('<I', offset + len(body))
        body += struct.pack('<II', len(data), len(packed)) + packed
        body += b'\0' * (-len(body) % 4)
    return bytes(table + body)


def literal(name, blob):
    safe = set(range(0x20, 0x7F)) - {ord('"'), ord('\\'), ord('?')}
    lines = [f'alignas(16) inline constexpr char {name}[] =']
    for i in range(0, len(blob), 120):
        lines.append('    "' + ''.join(chr(c) if c in safe else '\\%03o' % c for c in blob[i:i + 120]) + '"')
    lines[-1] += ';'
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', default=None, help='GTAVCS or GTALCS (default: both)')
    ap.add_argument('--only', default=None)
    ap.add_argument('--preview', default=None)
    ap.add_argument('--carlist', default=os.path.expanduser('~/Desktop/carlist/carlist'),
                    help='PS2 VCS texture dumps (native sizes); without it sizes follow the art (half of 256)')
    a = ap.parse_args()
    dumps = {}
    if os.path.isdir(a.carlist):
        for f in sorted(glob.glob(os.path.join(a.carlist, '*', '*.png'))):
            n = os.path.basename(f)[:-4]
            if n != 'lights':
                dumps.setdefault(n, f)
    plans = {}
    for plugin, game, depth, native_levels, hd_levels, alpha, kind in PLUGINS:
        if (a.game and game != a.game) or (a.only and plugin not in a.only.split(',')):
            continue
        models_path = os.path.join(ROOT, 'source', plugin, 'Models.hpp')
        if not os.path.exists(models_path):
            continue
        cache = models(models_path)
        count = max(cache) + 1
        native_blocks, hd_blocks, previews = [], [], []
        started = time.time()
        for c in range(count):
            name = cache[c]
            art_dir = os.path.join(ROOT, 'textures', game, 'replacements', name)
            if not os.path.isdir(art_dir):
                native_blocks.append(b''); hd_blocks.append(b'')
                continue
            art = png.read(os.path.join(art_dir, VARIANTS[0] + '.png'))
            size = native_size(name, art, dumps if game == 'GTAVCS' else {})
            hd = (min(art.shape[0], HD_LIMIT), min(art.shape[1], HD_LIMIT))
            # Plans are shared between the PS2 and PSP plugins of a game (all levels, cut per platform).
            for key, sz, d in (('native', size, depth), ('hd', hd, 8)):
                k = (game, name, key)
                if k not in plans:
                    plans[k] = plan(art_dir, sz, d, 8)
            native_blocks.append(encode(plans[(game, name, 'native')], native_levels, alpha))
            hd_blocks.append(encode(plans[(game, name, 'hd')], hd_levels, alpha))
            p = plans[(game, name, 'hd')]
            previews.append([p['pals'][v][p['idxs'][v][:p['w'] * p['h']]].reshape(p['h'], p['w'], 4) for v in range(8)])
        native_blob, hd_blob = pack_blob(native_blocks), pack_blob(hd_blocks)
        if kind == 'file':
            folder = os.path.join(ROOT, 'data', plugin, 'memstick', 'PSP', 'PLUGINS', plugin)
            os.makedirs(folder, exist_ok=True)
            open(os.path.join(folder, 'lights.bin'), 'wb').write(native_blob)
            open(os.path.join(folder, 'lights-hd.bin'), 'wb').write(hd_blob)
            where = folder
        else:
            where = os.path.join(ROOT, 'source', plugin, 'LightPack.hpp')
            lines = ['#pragma once', '// Generated by tools/vehicle-lights/pack.py (textures/<game>/build-lights.bat). Do not edit.',
                     '#include <cstdint>', 'namespace vehicle {',
                     f'// Native-size states, {len(native_blob)} bytes, and full-resolution 8-bit states, {len(hd_blob)} bytes.']
            lines += literal('lightPackData', native_blob) + literal('lightPackHdData', hd_blob)
            lines += [f'inline constexpr uint32_t lightPackSize = {len(native_blob)}, lightPackHdSize = {len(hd_blob)};',
                      'inline const uint8_t* lightPack() { return reinterpret_cast<const uint8_t*>(lightPackData); }',
                      'inline const uint8_t* lightPackHd() { return reinterpret_cast<const uint8_t*>(lightPackHdData); }',
                      '}', '']
            with open(where, 'w', newline='\n') as f:
                f.write('\n'.join(lines))
        sizes = [len(b) for b in hd_blocks if b]
        print(f'{plugin}: {len(sizes)} textures, native {len(native_blob)} bytes, hd {len(hd_blob)} bytes '
              f'(largest raw hd block {max(sizes)}), {time.time() - started:.0f}s -> {where}', flush=True)
        if a.preview:
            rows = []
            for p in previews:
                tiles = [np.pad(t[..., :3], ((0, 256 - t.shape[0]), (0, 256 - t.shape[1]), (0, 0))) for t in p[:4]]
                rows.append(np.concatenate(tiles, 1))
            png.write(os.path.join(a.preview, plugin + '-hd.png'), np.concatenate(rows, 0).astype(np.uint8))


if __name__ == '__main__':
    main()
