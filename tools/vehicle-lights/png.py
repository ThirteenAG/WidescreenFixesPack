# Minimal PNG reader/writer (no Pillow available). Returns numpy RGBA uint8 arrays.
import zlib, struct, numpy as np
def read(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', path
    pos = 8; idat = b''; plte = None; trns = None
    while pos < len(data):
        n, = struct.unpack('>I', data[pos:pos+4]); t = data[pos+4:pos+8]; c = data[pos+8:pos+8+n]; pos += 12 + n
        if t == b'IHDR': w, h, bd, ct, _, _, il = struct.unpack('>IIBBBBB', c)
        elif t == b'PLTE': plte = np.frombuffer(c, np.uint8).reshape(-1, 3)
        elif t == b'tRNS': trns = np.frombuffer(c, np.uint8)
        elif t == b'IDAT': idat += c
        elif t == b'IEND': break
    assert il == 0, 'interlaced'
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]
    bpp = max(1, ch * bd // 8)
    stride = (w * ch * bd + 7) // 8
    raw = np.frombuffer(zlib.decompress(idat), np.uint8)
    out = np.zeros((h, stride), np.uint8)
    prev = np.zeros(stride, np.int32)
    p = 0
    for y in range(h):
        f = raw[p]; line = raw[p+1:p+1+stride].astype(np.int32); p += 1 + stride
        if f == 0: cur = line
        elif f == 1:
            cur = line.copy()
            for i in range(bpp, stride): cur[i] = (cur[i] + cur[i-bpp]) & 255
        elif f == 2: cur = (line + prev) & 255
        elif f == 3:
            cur = line.copy()
            for i in range(stride): cur[i] = (cur[i] + (((cur[i-bpp] if i >= bpp else 0) + prev[i]) >> 1)) & 255
        elif f == 4:
            cur = line.copy()
            for i in range(stride):
                a = cur[i-bpp] if i >= bpp else 0; b = prev[i]; c = prev[i-bpp] if i >= bpp else 0
                pa = abs(b - c); pb = abs(a - c); pc = abs(a + b - 2*c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                cur[i] = (cur[i] + pr) & 255
        out[y] = cur; prev = cur
    if bd == 16: px = out.reshape(h, w, ch*2)[:, :, 0::2]
    elif bd == 8: px = out.reshape(h, w, ch)
    else:
        bits = np.unpackbits(out, axis=1)[:, :w*bd].reshape(h, w, bd)
        px = (bits * (1 << np.arange(bd-1, -1, -1))).sum(2).astype(np.uint8)[:, :, None]
    if ct == 3:
        pal = np.zeros((256, 4), np.uint8); pal[:, 3] = 255
        pal[:len(plte), :3] = plte
        if trns is not None: pal[:len(trns), 3] = trns
        return pal[px[:, :, 0]]
    if ct == 0: g = px[:, :, 0]; return np.dstack([g, g, g, np.full_like(g, 255)])
    if ct == 4: g = px[:, :, 0]; return np.dstack([g, g, g, px[:, :, 1]])
    if ct == 2: return np.dstack([px, np.full(px.shape[:2], 255, np.uint8)])
    return px.copy()
def write(path, rgba):
    rgba = np.ascontiguousarray(rgba.astype(np.uint8)); h, w = rgba.shape[:2]
    ch = rgba.shape[2] if rgba.ndim == 3 else 1
    ct = {1: 0, 3: 2, 4: 6}[ch]
    raw = b''.join(b'\x00' + rgba[y].tobytes() for y in range(h))
    def chunk(t, c): return struct.pack('>I', len(c)) + t + c + struct.pack('>I', zlib.crc32(t + c) & 0xffffffff)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, ct, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))
