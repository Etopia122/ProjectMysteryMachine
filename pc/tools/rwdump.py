#!/usr/bin/env python3
# Dump the RenderWare chunk tree of assets inside a HIP archive.
# usage: python pc/tools/rwdump.py <file.HIP> <asset type> [max assets] [name filter]
import struct, sys

NAMES = {
    0x01: 'Struct', 0x02: 'String', 0x03: 'Extension', 0x05: 'Camera', 0x06: 'Texture', 0x07: 'Material',
    0x08: 'MaterialList', 0x09: 'AtomicSect', 0x0A: 'PlaneSect', 0x0B: 'World', 0x0E: 'FrameList',
    0x0F: 'Geometry', 0x10: 'Clump', 0x12: 'Light', 0x14: 'Atomic', 0x15: 'TextureNative',
    0x16: 'TexDictionary', 0x1A: 'GeometryList', 0x1E: 'AnimAnimation', 0x116: 'Skin', 0x11E: 'HAnim',
    0x120: 'MatEffects', 0x11F: 'UserData', 0x50E: 'BinMesh', 0x510: 'NativeData', 0x253F2FE: 'Frame',
    0x11D: 'Collision', 0x0253F2F3: 'PipelineSet', 0x0253F2F6: 'SpecularMat', 0x0253F2F8: '2dEffect',
    0x0253F2F9: 'ExtraVertColour', 0x0253F2FC: 'ReflectionMat',
}

def hip_assets(d):
    out = []
    def walk(o, end):
        while o + 8 <= end:
            t = d[o:o + 4]
            n = struct.unpack('>I', d[o + 4:o + 8])[0]
            if t == b'AHDR':
                aid, typ, off, sz, plus, fl = struct.unpack('>I4sIIII', d[o + 8:o + 32])
                nm = ''
                if d[o + 32:o + 36] == b'ADBG':
                    nm = d[o + 44:o + 8 + n].split(b'\0')[0].decode('latin1')
                out.append((aid, typ.decode().strip(), off, sz, nm))
            elif t in (b'HIPA', b'PACK', b'DICT', b'ATOC', b'LTOC', b'STRM', b'LHDR', b'LDBG'):
                walk(o + 8, o + 8 + n)
            o += 8 + n
    walk(0, len(d))
    return out

def rw(d, o, end, depth, lines):
    while o + 12 <= end:
        t, n, v = struct.unpack('<III', d[o:o + 12])
        if n > end - o - 12:
            lines.append('  ' * depth + '?? bad chunk at %X (type %X size %X)' % (o, t, n))
            return
        name = NAMES.get(t, '%X' % t)
        extra = ''
        if t == 0x01 or n <= 16:
            extra = ' ' + d[o + 12:o + 12 + min(n, 24)].hex()
        lines.append('%s%s size=%X ver=%08X%s' % ('  ' * depth, name, n, v, extra))
        if t in (0x03, 0x07, 0x08, 0x0B, 0x0E, 0x0F, 0x10, 0x14, 0x16, 0x1A, 0x09, 0x0A, 0x06, 0x15, 0x12, 0x05):
            rw(d, o + 12, o + 12 + n, depth + 1, lines)
        o += 12 + n

d = open(sys.argv[1], 'rb').read()
typ = sys.argv[2]
mx = int(sys.argv[3]) if len(sys.argv) > 3 else 1
nf = sys.argv[4].lower() if len(sys.argv) > 4 else None
k = 0
for aid, t, off, sz, nm in hip_assets(d):
    if t != typ or (nf and nf not in nm.lower()):
        continue
    print('== %08X %s %s size %X' % (aid, t, nm, sz))
    lines = []
    rw(d, off, off + sz, 1, lines)
    print('\n'.join(lines[:80]))
    k += 1
    if k >= mx:
        break
