#!/usr/bin/env python3
# Extract the file system of a GameCube disc image (your own dump) into a folder
# the PC port reads its data from. Keeps the directory structure.
#
# usage: python pc/tools/extract_disc.py <game.iso> [outdir=game/files]
import os, struct, sys

iso = sys.argv[1]
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join('game', 'files')
f = open(iso, 'rb')
f.seek(0)
if f.read(6) != b'GIHE78':
    print('warning: disc id is not GIHE78 (Scooby-Doo! Night of 100 Frights, USA)')
f.seek(0x424)
fst_off, fst_size = struct.unpack('>II', f.read(8))
f.seek(fst_off)
fst = f.read(fst_size)
n = struct.unpack('>I', fst[8:12])[0]
strt = n * 12

def name(o):
    return fst[strt + o:fst.index(b'\0', strt + o)].decode('shift_jis')

total = 0
stack = [(n, '')]
for i in range(1, n):
    while stack and i >= stack[-1][0]:
        stack.pop()
    w0, a, b = struct.unpack('>III', fst[i * 12:i * 12 + 12])
    path = stack[-1][1] + name(w0 & 0xFFFFFF)
    if w0 >> 24:
        stack.append((b, path + '/'))
        os.makedirs(os.path.join(out, path), exist_ok=True)
        continue
    dst = os.path.join(out, path)
    os.makedirs(os.path.dirname(dst) or '.', exist_ok=True)
    f.seek(a)
    with open(dst, 'wb') as o:
        left = b
        while left:
            chunk = f.read(min(left, 1 << 20))
            o.write(chunk)
            left -= len(chunk)
    total += b
print('extracted %d entries, %.1f MB -> %s' % (n - 1, total / 1e6, out))
