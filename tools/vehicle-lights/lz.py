"""LZ4 block compression (format of the LZ4 block spec) for the ImVehLM packs.
Decoded by console::LightPack::Unpack. Greedy hash matcher; slow but offline."""


def compress(data):
    data = bytes(data)
    n = len(data)
    out = bytearray()
    table = {}
    anchor = 0
    i = 0
    limit = n - 12  # the last match must start 12 bytes before the end, the last 5 bytes are literals
    while i < limit:
        key = data[i:i + 4]
        candidate = table.get(key)
        table[key] = i
        if candidate is None or i - candidate > 65535:
            i += 1
            continue
        # extend the match
        length = 4
        end = n - 5
        while i + length < end and data[candidate + length] == data[i + length]:
            length += 1
        literals = data[anchor:i]
        _sequence(out, literals, i - candidate, length)
        for j in range(i + 1, min(i + length, limit)):
            table[data[j:j + 4]] = j
        i += length
        anchor = i
    _sequence(out, data[anchor:], 0, 0)
    return bytes(out)


def _sequence(out, literals, offset, length):
    lit = len(literals)
    match = length - 4 if length else 0
    token = (min(lit, 15) << 4) | (min(match, 15) if length else 0)
    out.append(token)
    if lit >= 15:
        r = lit - 15
        while r >= 255:
            out.append(255)
            r -= 255
        out.append(r)
    out += literals
    if not length:
        return
    out += offset.to_bytes(2, 'little')
    if match >= 15:
        r = match - 15
        while r >= 255:
            out.append(255)
            r -= 255
        out.append(r)


def decompress(src, size):
    out = bytearray()
    i = 0
    while i < len(src):
        token = src[i]; i += 1
        lit = token >> 4
        if lit == 15:
            while True:
                b = src[i]; i += 1; lit += b
                if b != 255:
                    break
        out += src[i:i + lit]; i += lit
        if i >= len(src):
            break
        offset = src[i] | (src[i + 1] << 8); i += 2
        match = (token & 15) + 4
        if (token & 15) == 15:
            while True:
                b = src[i]; i += 1; match += b
                if b != 255:
                    break
        for _ in range(match):
            out.append(out[-offset])
    assert len(out) == size
    return bytes(out)
