"""Palette quantisation helpers for tools/vehicle-lights/pack.py (numpy only)."""
import numpy as np

SAMPLES = 60000


def resize(img, h, w):
    """Premultiplied box filter to (h, w); nearest when enlarging."""
    img = img.astype(np.float64)
    if img.shape[:2] == (h, w):
        return img
    ky, kx = img.shape[0] // h, img.shape[1] // w
    if ky == 0 or kx == 0:
        ys = np.arange(h) * img.shape[0] // h
        xs = np.arange(w) * img.shape[1] // w
        return img[ys][:, xs]
    a = img[..., 3:4] / 255.0
    pre = np.concatenate([img[..., :3] * a, img[..., 3:4]], -1)
    pre = pre[:h * ky, :w * kx].reshape(h, ky, w, kx, 4).mean((1, 3))
    a = pre[..., 3:4] / 255.0
    rgb = np.where(a > 0, pre[..., :3] / np.maximum(a, 1e-6), 0)
    return np.concatenate([rgb, pre[..., 3:4]], -1)


def feat(px):
    """Premultiplied, luminance-weighted colour plus alpha."""
    a = px[:, 3:4] / 255.0
    return np.concatenate([px[:, :3] * a * np.array([0.30, 0.59, 0.11]) ** 0.5 * 1.7, px[:, 3:4] * 0.8], 1)


def nearest(f, c):
    out = np.empty(len(f), np.int64)
    for i in range(0, len(f), 32768):
        out[i:i + 32768] = ((f[i:i + 32768, None, :] - c[None]) ** 2).sum(-1).argmin(1)
    return out


def map_to(px, pal):
    return nearest(feat(px), feat(pal))


def _sample(px, w, rng):
    if len(px) <= SAMPLES:
        return px, w
    sel = rng.choice(len(px), SAMPLES, replace=False, p=w / w.sum())
    return px[sel], np.ones(SAMPLES)


def _centroid(sel, ww):
    a = sel[:, 3:4] / 255.0
    rgb = (sel[:, :3] * a * ww[:, None]).sum(0) / max((a[:, 0] * ww).sum(), 1e-6)
    return [*rgb, (sel[:, 3] * ww).sum() / ww.sum()]


def kmeans(px, k, weights=None, iters=16, seed=0):
    rng = np.random.default_rng(seed)
    w = np.ones(len(px)) if weights is None else weights.astype(np.float64)
    px, w = _sample(px, w, rng)
    f = feat(px)
    n = len(f)
    k = min(k, n)
    c = [f[rng.integers(n)]]
    d = ((f - c[0]) ** 2).sum(1)
    for _ in range(1, k):
        p = d * w
        c.append(f[rng.choice(n, p=p / p.sum())] if p.sum() > 0 else f[rng.integers(n)])
        d = np.minimum(d, ((f - c[-1]) ** 2).sum(1))
    c = np.array(c)
    for _ in range(iters):
        lab = nearest(f, c)
        for j in range(k):
            m = lab == j
            if m.any():
                c[j] = (f[m] * w[m, None]).sum(0) / w[m].sum()
    lab = nearest(f, c)
    pal = np.zeros((k, 4))
    for j in range(k):
        m = lab == j
        if m.any():
            pal[j] = _centroid(px[m], w[m])
    return np.clip(np.round(pal), 0, 255)


def kmeans_fixed(px, pal, free, iters=12, seed=0):
    """Refine only the entries in `free` to fit `px`; the others stay."""
    pal = pal.astype(np.float64).copy()
    if len(px) == 0 or not len(free):
        return pal
    rng = np.random.default_rng(seed)
    px, w = _sample(px, np.ones(len(px)), rng)
    f = feat(px)
    for _ in range(iters):
        lab = nearest(f, feat(pal))
        for k in free:
            m = lab == k
            if m.any():
                pal[k] = _centroid(px[m], w[m])
            else:
                dist = ((f - feat(pal)[lab]) ** 2).sum(1)
                pal[k] = px[dist.argmax()]
    return np.clip(np.round(pal), 0, 255)


def plan_variants(levels_v, k, threshold=24.0):
    """levels_v[v] is a list of RGBA float arrays (one per mip level).
    Returns per-variant palettes and linear index arrays (levels concatenated).
    Every variant keeps the base variant's index wherever its texel did not
    change, so the variants are stored as small differences."""
    flat = [np.concatenate([lv.reshape(-1, 4) for lv in L]) for L in levels_v]
    base = flat[0]
    spread = np.stack(flat).std(0).sum(1)
    pal0 = kmeans(np.concatenate(flat), k, np.tile(1 + (spread > 8) * 2.0, len(flat)))
    # Fit the base on its own texels, starting from the shared palette.
    pal0 = kmeans_fixed(base, pal0, list(range(len(pal0))), iters=6)
    idx0 = map_to(base, pal0)
    pals, idxs = [pal0], [idx0]
    for v in range(1, len(flat)):
        px = flat[v]
        changed = np.abs(px - base).sum(1) > threshold
        keep = set(np.unique(idx0[~changed]).tolist())
        free = [j for j in range(len(pal0)) if j not in keep]
        pal = kmeans_fixed(px[changed], pal0, free) if changed.any() else pal0.copy()
        idx = idx0.copy()
        if changed.any():
            idx[changed] = map_to(px[changed], pal)
        pals.append(pal)
        idxs.append(idx)
    return pals, idxs
