#!/usr/bin/env python3
# Compare the data sections of our compiled object with the original (asm) object
# for one unit, ignoring relocated words. Prints size and first differing offset.
# usage: tools/seccmp.py Game/zEntShaggy [section ...]
import struct, sys

def sections(path):
    d = open(path, 'rb').read()
    shoff = struct.unpack('>I', d[0x20:0x24])[0]
    es, n, si = struct.unpack('>HHH', d[0x2E:0x34])
    sh = [struct.unpack('>IIIIIIIIII', d[shoff + i * es:shoff + (i + 1) * es]) for i in range(n)]
    strtab = sh[si][4]
    out = {}
    for i, s in enumerate(sh):
        name = d[strtab + s[0]:d.index(b'\0', strtab + s[0])].decode()
        out[name] = (s, d[s[4]:s[4] + s[5]] if s[1] != 8 else b'\0' * s[5], i)
    relocs = {}
    for name, (s, data, i) in out.items():
        if s[1] == 4:  # RELA
            tgt = s[7]
            offs = {struct.unpack('>I', data[k:k + 4])[0] for k in range(0, len(data), 12)}
            relocs[tgt] = offs
    return out, relocs

unit = sys.argv[1]
want = sys.argv[2:] or ['.rodata', '.data', '.bss', '.sdata', '.sdata2', '.sbss', '.ctors']
ours, ro = sections('build/GIHE78/src/%s.o' % unit)
orig, rg = sections('build/GIHE78/obj/%s.o' % unit)
for sec in want:
    if sec not in ours and sec not in orig:
        continue
    a = ours.get(sec, (None, b'', -1)); b = orig.get(sec, (None, b'', -1))
    da, db = bytearray(a[1]), bytearray(b[1])
    for k in ro.get(a[2], ()):
        da[k:k + 4] = b'\0' * 4 if k + 4 <= len(da) else da[k:k + 4]
    for k in rg.get(b[2], ()):
        db[k:k + 4] = b'\0' * 4 if k + 4 <= len(db) else db[k:k + 4]
    diff = next((k for k in range(min(len(da), len(db))) if da[k] != db[k]), None)
    status = 'OK' if da == db else 'first diff at 0x%X' % diff if diff is not None else 'size differs'
    print('%-8s ours 0x%-6X orig 0x%-6X %s' % (sec, len(da), len(db), status))
    if diff is not None and da != db:
        print('   ours %s\n   orig %s' % (bytes(da[diff:diff + 24]), bytes(db[diff:diff + 24])))
