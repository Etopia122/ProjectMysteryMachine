#!/usr/bin/env python3
# Tile downscaled 32-bit BMP screenshots into one PNG (3 per row).
# usage: python pc/tools/tile.py out.png scale a.bmp b.bmp ...
import struct, sys, zlib

out, S, files = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
tiles = []
for f in files:
    d = open(f, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    rows = []
    for y in range(0, h, S):
        sy = h - 1 - y
        r = bytearray()
        for x in range(0, w, S):
            i = off + (sy * w + x) * 4
            r += bytes((d[i + 2], d[i + 1], d[i]))
        rows.append(bytes(r))
    tiles.append(rows)
tw, th, cols = len(tiles[0][0]) // 3, len(tiles[0]), min(3, len(tiles))
W, H = cols * tw, ((len(tiles) + cols - 1) // cols) * th
img = [bytearray(W * 3) for _ in range(H)]
for k, t in enumerate(tiles):
    ox, oy = (k % cols) * tw * 3, (k // cols) * th
    for y, r in enumerate(t):
        img[oy + y][ox:ox + tw * 3] = r
raw = b''.join(b'\0' + bytes(r) for r in img)
def chunk(t, b):
    return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xffffffff)
open(out, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0)) +
                      chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
print('%dx%d -> %s' % (W, H, out))
