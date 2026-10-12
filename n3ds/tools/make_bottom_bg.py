#!/usr/bin/env python3
# Converts a picture into the New 3DS port's bottom screen background: 320x240
# RGBA ("SBG1", u32 width, u32 height, then the pixels, little-endian), which
# the app reads from romfs:/bottom_bg.rgba (the CIA) or sdmc:/scooby/bottom_bg.rgba.
# The picture (from the player's own game art) stays out of the repository:
# make_cia.sh converts game/n3ds_bottom_bg.png when it is there.
# usage: python n3ds/tools/make_bottom_bg.py <in.png> <out.rgba>
# Reads 8-bit non-interlaced RGB or RGBA PNGs; scales to 320x240 by averaging.
import struct, sys, zlib

W, H = 320, 240


def read_png(path):
    d = open(path, 'rb').read()
    if d[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('not a PNG')
    o, idat = 8, b''
    w = h = ctype = 0
    while o < len(d):
        n, t = struct.unpack('>I4s', d[o:o + 8])
        body = d[o + 8:o + 8 + n]
        if t == b'IHDR':
            w, h, depth, ctype, _, _, interlace = struct.unpack('>IIBBBBB', body)
            if depth != 8 or ctype not in (2, 6) or interlace:
                raise ValueError('need an 8-bit, non-interlaced RGB or RGBA PNG')
        elif t == b'IDAT':
            idat += body
        o += 12 + n
    bpp = 3 if ctype == 2 else 4
    raw = zlib.decompress(idat)
    stride = w * bpp
    rows, prev, p = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[p]
        line = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line)
        prev = line
    px = [[tuple(r[x * bpp:x * bpp + 3]) + ((r[x * bpp + 3],) if bpp == 4 else (255,)) for x in range(w)]
          for r in rows]
    return w, h, px


def main():
    w, h, px = read_png(sys.argv[1])
    out = bytearray(b'SBG1' + struct.pack('<II', W, H))
    for y in range(H):
        y0, y1 = y * h // H, max(y * h // H + 1, (y + 1) * h // H)
        for x in range(W):
            x0, x1 = x * w // W, max(x * w // W + 1, (x + 1) * w // W)
            n = (y1 - y0) * (x1 - x0)
            acc = [0, 0, 0, 0]
            for yy in range(y0, y1):
                for xx in range(x0, x1):
                    for c in range(4):
                        acc[c] += px[yy][xx][c]
            out += bytes((v + n // 2) // n for v in acc)
    open(sys.argv[2], 'wb').write(out)
    print('%s: %dx%d -> %dx%d' % (sys.argv[2], w, h, W, H))


if __name__ == '__main__':
    main()
