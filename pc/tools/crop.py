#!/usr/bin/env python3
# Crop (and enlarge) a region of a 32-bit BMP screenshot into a PNG.
# usage: python pc/tools/crop.py in.bmp out.png x0 y0 x1 y1 [scale]
import struct, sys, zlib

src, out = sys.argv[1], sys.argv[2]
x0, y0, x1, y1 = map(int, sys.argv[3:7])
S = int(sys.argv[7]) if len(sys.argv) > 7 else 1
d = open(src, 'rb').read()
off = struct.unpack_from('<I', d, 10)[0]
w, h = struct.unpack_from('<ii', d, 18)
rows = []
for y in range(y0, y1):
    sy = h - 1 - y
    r = bytearray()
    for x in range(x0, x1):
        i = off + (sy * w + x) * 4
        r += bytes((d[i + 2], d[i + 1], d[i])) * S
    rows += [bytes(r)] * S
def chunk(t, b):
    return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xffffffff)
raw = b''.join(b'\0' + r for r in rows)
open(out, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', (x1 - x0) * S, len(rows), 8, 2, 0, 0, 0)) +
                      chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
