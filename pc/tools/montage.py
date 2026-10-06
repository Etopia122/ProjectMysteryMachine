#!/usr/bin/env python3
# Crop the same region from several 32-bit BMP screenshots and stack them side by side into one PNG.
# usage: python pc/tools/montage.py out.png x0 y0 x1 y1 a.bmp b.bmp ...
import struct, sys, zlib

def load(path):
    d = open(path, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    bpp = struct.unpack_from('<H', d, 28)[0] // 8
    flip = h > 0
    h = abs(h)
    rows = []
    for y in range(h):
        sy = h - 1 - y if flip else y
        r = d[off + sy * w * bpp: off + (sy + 1) * w * bpp]
        rows.append(bytes(b for i in range(w) for b in (r[i * bpp + 2], r[i * bpp + 1], r[i * bpp])))
    return w, h, rows

out, x0, y0, x1, y1 = sys.argv[1], *map(int, sys.argv[2:6])
imgs = [load(p) for p in sys.argv[6:]]
cw, ch = x1 - x0, y1 - y0
rows = []
for y in range(y0, y1):
    rows.append(b''.join(img[2][y][x0 * 3:x1 * 3] + b'\xff\xff\xff' * 4 for img in imgs))
W = (cw + 4) * len(imgs)
raw = b''.join(b'\0' + r for r in rows)
def chunk(t, b):
    return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xffffffff)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, ch, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
open(out, 'wb').write(png)
print('%dx%d -> %s' % (W, ch, out))
