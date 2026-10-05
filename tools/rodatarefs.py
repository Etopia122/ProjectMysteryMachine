#!/usr/bin/env python3
# For one unit, list per function the offsets into a data section (default .rodata)
# that the function references, in our object and in the original object. Shows
# which functions reference data the other side lays out differently.
# usage: tools/rodatarefs.py Game/zEntVillain [.rodata] [min offset]
import collections
import struct
import sys

R_PPC_ADDR16_LO, R_PPC_ADDR16_HA, R_PPC_EMB_SDA21 = 4, 6, 109


def load(path):
    d = open(path, 'rb').read()
    shoff = struct.unpack('>I', d[0x20:0x24])[0]
    es, n, si = struct.unpack('>HHH', d[0x2E:0x34])
    sh = [struct.unpack('>IIIIIIIIII', d[shoff + i * es:shoff + (i + 1) * es]) for i in range(n)]
    st = sh[si][4]
    names = [d[st + s[0]:d.index(b'\0', st + s[0])].decode() for s in sh]
    syms = []
    for s in sh:
        if s[1] == 2:
            strtab = sh[s[6]][4]
            for j in range(s[5] // 16):
                nm, val, sz, info, oth, shn = struct.unpack('>IIIBBH', d[s[4] + j * 16:s[4] + j * 16 + 16])
                syms.append((d[strtab + nm:d.index(b'\0', strtab + nm)].decode(), val, sz, info, shn))
    refs = collections.defaultdict(list)
    text = names.index('.text')
    funcs = sorted((v, z, nm) for nm, v, z, info, shn in syms if shn == text and info & 0xF == 2)
    for i, s in enumerate(sh):
        if s[1] != 4 or s[7] != text:
            continue
        for k in range(0, s[5], 12):
            off, info, add = struct.unpack('>IIi', d[s[4] + k:s[4] + k + 12])
            typ = info & 0xFF
            if typ != R_PPC_ADDR16_LO:
                continue
            sym = syms[info >> 8]
            if sym[4] >= len(names):
                continue
            secname = names[sym[4]]
            fn = next((nm for v, z, nm in funcs if v <= off < v + z), '?')
            refs[(fn, secname)].append(sym[1] + add)
    return refs, names


unit = sys.argv[1]
sec = sys.argv[2] if len(sys.argv) > 2 else '.rodata'
lo = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
ours, _ = load('build/GIHE78/src/%s.o' % unit)
orig, _ = load('build/GIHE78/obj/%s.o' % unit)
fns = sorted({f for f, s in list(ours) + list(orig) if s == sec})
for f in fns:
    a = sorted({x for x in ours.get((f, sec), []) if x >= lo})
    b = sorted({x for x in orig.get((f, sec), []) if x >= lo})
    if a == b:
        continue
    print('%-50s ours %s' % (f[:50], ' '.join('%X' % x for x in a)))
    print('%-50s orig %s' % ('', ' '.join('%X' % x for x in b)))
