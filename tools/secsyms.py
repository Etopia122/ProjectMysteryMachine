#!/usr/bin/env python3
# List symbols in a section of an object file (offset, size, name), sorted by offset.
# usage: tools/secsyms.py build/GIHE78/src/Game/zEntVillain.o .rodata [minoff]
import struct, sys
p, want = sys.argv[1], sys.argv[2]
lo = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
d = open(p, 'rb').read()
shoff = struct.unpack('>I', d[0x20:0x24])[0]; es, n, si = struct.unpack('>HHH', d[0x2E:0x34])
sh = [struct.unpack('>IIIIIIIIII', d[shoff + i * es:shoff + (i + 1) * es]) for i in range(n)]; st = sh[si][4]
names = [d[st + s[0]:d.index(b'\0', st + s[0])].decode() for s in sh]
idx = names.index(want)
for s in sh:
    if s[1] == 2:
        strtab = sh[s[6]][4]
        rows = []
        for j in range(s[5] // 16):
            nm, val, sz, info, oth, shn = struct.unpack('>IIIBBH', d[s[4] + j * 16:s[4] + j * 16 + 16])
            if shn == idx and val >= lo:
                rows.append((val, sz, d[strtab + nm:d.index(b'\0', strtab + nm)].decode()))
        for r in sorted(rows):
            print('0x%05X 0x%-5X %s' % r)
