"""Offline generator for the Stories ImVehLM plugins.

Builds per-texture light rectangles (headlight / tail / brake / reverse) from
  * the hand-painted replacement packs in textures/GTAVCS and textures/GTALCS
    (each lit variant is diffed against the matching "off" variant), and
  * for PS2 VCS additionally the hand-drawn outlines in carlist/<car>/lights.png.
Writes a compact LightRects.hpp into each plugin folder, indexed by the cache
ids of that plugin's Models.hpp. Nothing here is needed at runtime.

usage: python tools/vehicle-lights/generate.py [--carlist DIR] [--preview DIR] [--only PLUGIN[,PLUGIN]]
"""
import argparse, os, re, sys, glob
from collections import deque
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
VARIANTS = ['headl_off__taillights_off', 'headl_off__taillights_on', 'headl_off__taillights_break', 'headl_off__taillights_reverse',
            'headl_on__taillights_off', 'headl_on__taillights_on', 'headl_on__taillights_break', 'headl_on__taillights_reverse']
HEAD, TAIL, TAILREV, BRAKE, REVERSE = range(5)
ROLE_NAMES = ['Head', 'Tail', 'TailReverse', 'Brake', 'Reverse']
THRESHOLD = 24.0   # mean |RGB| sum difference per native texel


def down(img, h, w):
    ky, kx = img.shape[0] // h, img.shape[1] // w
    return img[:h * ky, :w * kx].reshape(h, ky, w, kx, *img.shape[2:]).mean((1, 3))


def models(path):
    out = {}
    for line in open(path):
        m = re.match(r'\s*\{"([^"]+)",\s*(-?\d+)\}', line)
        if m and int(m.group(2)) >= 0:
            out[int(m.group(2))] = m.group(1)
    return out


def pack_roles(pack_dir, h, w):
    """Role map (h,w) from the replacement variants; -1 = not a light."""
    vs = [png.read(os.path.join(pack_dir, v + '.png')).astype(np.float32)[..., :3] for v in VARIANTS]
    d = lambda a, b: np.abs(vs[a] - vs[b]).sum(-1)
    head = np.maximum(d(4, 0), np.maximum(d(5, 1), d(6, 2)))
    tail = np.maximum(d(1, 0), d(5, 4))
    brake = np.maximum(d(2, 0), d(6, 4))
    rev = np.maximum(d(3, 0), d(7, 4))
    head, tail, brake, rev = [down(x, h, w) for x in (head, tail, brake, rev)]
    H, T, B, R = head > THRESHOLD, tail > THRESHOLD, brake > THRESHOLD, rev > THRESHOLD
    roles = np.full((h, w), -1, np.int8)
    roles[R] = REVERSE
    roles[B] = BRAKE
    roles[T] = TAIL
    roles[T & R] = TAILREV
    # A texel the painter lit with the headlights is a headlight unless a rear change is stronger.
    roles[H & (head >= np.maximum(np.maximum(tail, brake), rev))] = HEAD
    roles[H & ~(T | B | R)] = HEAD
    return roles


def flood_regions(wall):
    """Label 4-connected regions of ~wall; returns label image (0 = wall)."""
    h, w = wall.shape
    lab = np.zeros((h, w), np.int32)
    n = 0
    for y in range(h):
        for x in range(w):
            if not wall[y, x] and not lab[y, x]:
                n += 1
                lab[y, x] = n
                q = deque([(y, x)])
                while q:
                    cy, cx = q.popleft()
                    for ny, nx in ((cy + 1, cx), (cy - 1, cx), (cy, cx + 1), (cy, cx - 1)):
                        if 0 <= ny < h and 0 <= nx < w and not wall[ny, nx] and not lab[ny, nx]:
                            lab[ny, nx] = n
                            q.append((ny, nx))
    return lab, n


def outline_shapes(lights, h, w):
    """Filled shapes outlined in green in lights.png, at native (h,w). Returns a list of boolean masks."""
    g = (lights[..., 3] > 64) & (lights[..., 1] > 128) & (lights[..., 0] < 120) & (lights[..., 2] < 120)
    lab, n = flood_regions(g)
    H, W = g.shape
    shapes = []
    for i in range(1, n + 1):
        m = lab == i
        ys, xs = np.nonzero(m)
        # The background region reaches three or more borders or covers most of the image.
        sides = int(ys.min() == 0) + int(ys.max() == H - 1) + int(xs.min() == 0) + int(xs.max() == W - 1)
        if sides >= 3 or m.sum() > 0.5 * H * W:
            continue
        # Include the outline itself.
        d = m.copy()
        d[1:] |= m[:-1]; d[:-1] |= m[1:]; d[:, 1:] |= m[:, :-1]; d[:, :-1] |= m[:, 1:]
        d &= (m | g)
        nat = down(d.astype(np.float32), h, w) >= 0.5
        if nat.sum():
            shapes.append(nat)
    return shapes


def hue_class(rgb):
    """'red', 'amber' or 'white' for a mean colour, None if too dark or unclear."""
    r, g, b = [float(x) for x in rgb]
    mx, mn = max(r, g, b), min(r, g, b)
    if mx < 24:
        return None
    s = (mx - mn) / mx
    if s < 0.35:
        return 'white'
    if r >= g and r >= b:
        hue = 60.0 * (g - b) / (mx - mn + 1e-6)
        if hue < 22:
            return 'red'
        if hue < 55:
            return 'amber' if s > 0.55 else 'white'
    return None


def merge_outlines(roles, base, shapes):
    """Union the hand-drawn outlines into the role map. Outlines the painter lit take the painted role,
    unlit ones are classified by their colour (red = tail, white/pale yellow = headlight, amber = indicator)."""
    out = roles.copy()
    tail_role = TAILREV if (roles == TAILREV).sum() >= (roles == TAIL).sum() else TAIL
    report = []
    for m in shapes:
        inside = roles[m]
        lit = inside[inside >= 0]
        if len(lit) * 4 >= m.sum():
            role = int(np.bincount(lit, minlength=5).argmax())
            how = 'pack'
        else:
            c = hue_class(base[m][..., :3].mean(0))
            role = {'red': tail_role, 'white': HEAD}.get(c)
            how = 'colour:' + str(c)
        report.append((int(m.sum()), how, ROLE_NAMES[role] if role is not None else 'skip'))
        if role is not None:
            out[m & (out < 0)] = role
    return out, report


def clean(roles):
    """Drop isolated single texels (noise from repainting)."""
    out = roles.copy()
    h, w = roles.shape
    for y in range(h):
        for x in range(w):
            if roles[y, x] < 0:
                continue
            n = sum(1 for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1))
                    if 0 <= ny < h and 0 <= nx < w and roles[ny, nx] >= 0)
            if n == 0:
                out[y, x] = -1
    return out


def rects(roles):
    """Greedy decomposition into disjoint rectangles (x0, y0, x1, y1 inclusive, role)."""
    h, w = roles.shape
    used = np.zeros((h, w), bool)
    out = []
    for y in range(h):
        for x in range(w):
            r = roles[y, x]
            if r < 0 or used[y, x]:
                continue
            x1 = x
            while x1 + 1 < w and roles[y, x1 + 1] == r and not used[y, x1 + 1]:
                x1 += 1
            y1 = y
            while y1 + 1 < h and (roles[y1 + 1, x:x1 + 1] == r).all() and not used[y1 + 1, x:x1 + 1].any():
                y1 += 1
            used[y:y1 + 1, x:x1 + 1] = True
            out.append((x, y, x1, y1, int(r)))
    return out


def build(plugin, pack, dumps, outlines):
    cache = models(os.path.join(ROOT, 'source', plugin, 'Models.hpp'))
    entries, previews, log = [], [], []
    for idx in range(max(cache) + 1):
        name = cache[idx]
        pdir = os.path.join(pack, name)
        if not os.path.isdir(pdir):
            entries.append((name, 0, 0, []))
            log.append(f'{name}: no replacement art, no lights')
            continue
        off = png.read(os.path.join(pdir, VARIANTS[0] + '.png'))
        if name in dumps:
            base = png.read(dumps[name][0])
            h, w = base.shape[:2]
        else:
            h, w = off.shape[:2]
            if max(h, w) >= 256:
                h, w = h // 2, w // 2
            base = down(off.astype(np.float32), h, w)
        roles = pack_roles(pdir, h, w)
        note = ''
        if outlines and name in dumps:
            shapes = []
            for f in dumps[name]:
                lp = os.path.join(os.path.dirname(f), 'lights.png')
                if os.path.exists(lp):
                    shapes += outline_shapes(png.read(lp), h, w)
            if shapes:
                roles, report = merge_outlines(roles, base, shapes)
                note = '; outlines: ' + ', '.join(f'{n}px {how} -> {role}' for n, how, role in report)
        roles = clean(roles)
        rs = rects(roles)
        entries.append((name, w, h, rs))
        log.append(f'{name} {w}x{h}: {len(rs)} rects, ' + ' '.join(f'{ROLE_NAMES[r]}={(roles == r).sum()}' for r in range(5)) + note)
        previews.append((name, base, roles))
    return entries, previews, log


def emit(plugin, entries, sources, header='../Shared/Console/VehicleLights.hpp'):
    roles = ['console::LightHead', 'console::LightTail', 'console::LightTailReverse', 'console::LightBrake', 'console::LightReverse']
    lines = ['#pragma once', f'// Generated by tools/vehicle-lights/generate.py from {sources}. Do not edit.',
             f'#include "{header}"', 'namespace vehicle {',
             '// Light rectangles in texels of the reference size (inclusive bounds).',
             'inline constexpr console::LightRect lightRects[] = {']
    sets, first = [], 0
    for name, w, h, rs in entries:
        if rs:
            lines.append(f'    // {name}')
        for i in range(0, len(rs), 4):
            lines.append('    ' + ' '.join(f'{{{x0}, {y0}, {x1}, {y1}, {roles[r]}}},' for x0, y0, x1, y1, r in rs[i:i + 4]))
        sets.append(f'    {{{first}, {len(rs)}, {w if rs else 0}, {h if rs else 0}}}, // {name}')
        first += len(rs)
    if not first:
        lines.append('    {0, 0, 0, 0, console::LightHead},')
    lines += ['};', '// Indexed by the cache ids in Models.hpp: first rect, count, reference width and height.',
              'inline constexpr console::LightSet lightSets[] = {'] + sets + ['};', '}', '']
    path = os.path.join(ROOT, 'source', plugin, 'LightRects.hpp')
    with open(path, 'w', newline='\n') as f:
        f.write('\n'.join(lines))
    return path, first


def preview(previews, path):
    colours = np.array([[255, 255, 160], [255, 0, 0], [255, 0, 255], [255, 140, 0], [80, 160, 255]], np.float32)
    tiles = []
    for name, base, roles in previews:
        b = base.astype(np.float32)[..., :3]
        k = max(1, 128 // max(roles.shape))
        b = b.repeat(k, 0).repeat(k, 1)
        r = roles.repeat(k, 0).repeat(k, 1)
        m = r >= 0
        b[m] = b[m] * 0.35 + colours[r[m]] * 0.65
        t = np.zeros((128, 128, 3), np.float32)
        t[:min(128, b.shape[0]), :min(128, b.shape[1])] = b[:128, :128]
        tiles.append(t)
    pad = np.zeros((128, 128, 3), np.float32)
    rows = [np.concatenate(tiles[i:i + 10] + [pad] * (10 - len(tiles[i:i + 10])), 1) for i in range(0, len(tiles), 10)]
    png.write(path, np.concatenate(rows, 0).astype(np.uint8))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--carlist', default=os.path.expanduser('~/Desktop/carlist/carlist'),
                    help='folder of PS2 VCS texture dumps with lights.png outlines, one folder per car')
    ap.add_argument('--preview', default=None, help='write a role preview png per plugin into this folder')
    ap.add_argument('--only', default=None, help='comma separated plugin folders to regenerate (default: all)')
    a = ap.parse_args()
    dumps = {}
    if os.path.isdir(a.carlist):
        for f in sorted(glob.glob(os.path.join(a.carlist, '*', '*.png'))):
            n = os.path.basename(f)[:-4]
            if n != 'lights':
                dumps.setdefault(n, []).append(f)
    vcs = os.path.join(ROOT, 'textures', 'GTAVCS', 'replacements')
    lcs = os.path.join(ROOT, 'textures', 'GTALCS', 'replacements')
    jobs = [('GTAVCS.PCSX2F.ImVehLM', vcs, True, 'textures/GTAVCS and the carlist lights.png outlines'),
            ('GTAVCS.PPSSPP.ImVehLM', vcs, False, 'textures/GTAVCS'),
            ('GTALCS.PPSSPP.ImVehLM', lcs, False, 'textures/GTALCS')]
    if os.path.isfile(os.path.join(ROOT, 'source', 'GTALCS.PCSX2F.ImVehLM', 'Models.hpp')):
        jobs.append(('GTALCS.PCSX2F.ImVehLM', lcs, False, 'textures/GTALCS'))
    if a.only:
        jobs = [j for j in jobs if j[0] in a.only.split(',')]
    for plugin, pack, outlines, src, *header in jobs:
        # VCS dumps give the native sizes for both platforms (PSP and PS2 car textures match);
        # LCS textures are 128x128 with 256x256 replacement art.
        entries, previews, log = build(plugin, pack, dumps if 'VCS' in plugin else {}, outlines and bool(dumps))
        path, count = emit(plugin, entries, src, *header)
        print(f'{plugin}: {count} rects -> {path}')
        for line in log:
            print('   ', line)
        if a.preview:
            preview(previews, os.path.join(a.preview, plugin + '.png'))


if __name__ == '__main__':
    main()
