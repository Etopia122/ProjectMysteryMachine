#!/usr/bin/env python3
# Convert a 24/32-bit BMP (as saved by SDL) to PNG using only the standard library.
# usage: python pc/tools/bmp2png.py in.bmp out.png [scale-down factor]
import struct, sys, zlib
src, dst = sys.argv[1], sys.argv[2]
k = int(sys.argv[3]) if len(sys.argv) > 3 else 1
d = open(src, 'rb').read()
off = struct.unpack_from('<I', d, 10)[0]
w, h = struct.unpack_from('<ii', d, 18)
bpp = struct.unpack_from('<H', d, 28)[0] // 8
stride = (w * bpp + 3) & ~3
flip = h > 0
h = abs(h)
rows = []
for y in range(0, h, k):
    sy = h - 1 - y if flip else y
    r = d[off + sy * stride: off + sy * stride + w * bpp]
    out = bytearray()
    for x in range(0, w, k):
        b, g, rr = r[x * bpp], r[x * bpp + 1], r[x * bpp + 2]
        out += bytes((rr, g, b))
    rows.append(b'\0' + bytes(out))
W, H = (w + k - 1) // k, len(rows)
def chunk(t, data):
    return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
png += chunk(b'IDAT', zlib.compress(b''.join(rows), 6)) + chunk(b'IEND', b'')
open(dst, 'wb').write(png)
print('%dx%d -> %s' % (W, H, dst))
