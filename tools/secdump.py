#!/usr/bin/env python3
# Dump one section of our object and of the original object as 4-byte words side by side,
# with float interpretation; marks differing words.
# usage: tools/secdump.py Game/zEntPlayer .sdata2 [start] [end] [shift]   (shift: orig offset - ours offset)
import struct, sys
def sec(p, name):
    d = open(p, 'rb').read(); shoff = struct.unpack('>I', d[0x20:0x24])[0]; es, n, si = struct.unpack('>HHH', d[0x2E:0x34])
    sh = [struct.unpack('>IIIIIIIIII', d[shoff + i * es:shoff + (i + 1) * es]) for i in range(n)]; st = sh[si][4]
    for s in sh:
        if d[st + s[0]:d.index(b'\0', st + s[0])].decode() == name: return d[s[4]:s[4] + s[5]]
    return b''
u, name = sys.argv[1], sys.argv[2]
a, b = sec('build/GIHE78/src/%s.o' % u, name), sec('build/GIHE78/obj/%s.o' % u, name)
lo = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
hi = int(sys.argv[4], 0) if len(sys.argv) > 4 else max(len(a), len(b))
sh = int(sys.argv[5], 0) if len(sys.argv) > 5 else 0
for o in range(lo, hi, 4):
    wa, wb = a[o:o + 4], b[o + sh:o + sh + 4]
    fa = '%g' % struct.unpack('>f', wa)[0] if len(wa) == 4 else ''
    fb = '%g' % struct.unpack('>f', wb)[0] if len(wb) == 4 else ''
    print('%s %04X  %-8s %-14s | %-8s %-14s' % ('!!' if wa != wb else '  ', o, wa.hex(), fa, wb.hex(), fb))
