#!/usr/bin/env python3
# Convert a dumped .rgba texture (int32 w, int32 h, RGBA8 pixels) to PNG, alpha shown as a checkerboard.
import struct, sys, zlib
d = open(sys.argv[1], 'rb').read()
w, h = struct.unpack_from('<ii', d, 0)
px = d[8:]
rows = []
for y in range(h):
    out = bytearray(b'\0')
    for x in range(w):
        r, g, b, a = px[(y * w + x) * 4:(y * w + x) * 4 + 4]
        bg = 200 if ((x // 8) + (y // 8)) & 1 else 120
        out += bytes(((r * a + bg * (255 - a)) // 255, (g * a + bg * (255 - a)) // 255, (b * a + bg * (255 - a)) // 255))
    rows.append(bytes(out))
def chunk(t, data):
    return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
open(sys.argv[2], 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                              chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))
