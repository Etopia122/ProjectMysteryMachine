#!/usr/bin/env python3
# Byte layouts of the game's HIP asset types (big-endian on GameCube) and a
# validator that checks them against real disc data.
#
# Layout language (one string per fixed part):
#   w = 32-bit field, h = 16-bit field, b = byte (not swapped), d = 64-bit field
#   a digit prefix repeats the next code ("12w" = twelve words)
#   E = p2EntAsset (0x50 bytes), B = p2BaseAsset (8 bytes), M = p2EntMotionAsset (0x30 bytes)
# Each type is a list of steps:
#   ('fix', layout)                 fixed-size part
#   ('arr', count_expr, layout)     array; count_expr is evaluated on already-read fields
#   ('links',)                      p2LinkAsset[linkCount] (32 bytes each), must end the asset
#   ('rest', layout)                remaining bytes as repeated layout
#   ('custom', name)                handled by hand-written code
#
# usage: python pc/tools/asset_layouts.py validate [TYPE...]
#        python pc/tools/asset_layouts.py gen > pc/src/asset_layouts_gen.inc
import collections, glob, re, struct, sys

MACROS = {
    'B': 'wbbh',
    'E': 'wbbh' + 'bbbb' + 'w' + '9w' + '5w' + '2w',
    'M': 'm',             # p2EntMotionAsset (0x30): union layout depends on its type byte
}

TYPES = {
    'TRIG': [('fix', 'E16w'), ('links',)],
    'MVPT': [('fix', 'B3whbbbbhw'), ('arr', 'f[12]', 'w'), ('links',)],
    'PKUP': [('fix', 'Ewhh'), ('links',)],
    'SIMP': [('fix', 'E2wbbbb'), ('links',)],
    'PLAT': [('fix', 'Ebbh9w'), ('fix', 'M'), ('links',)],
    'PEND': [('fix', 'E'), ('fix', 'M'), ('links',)],
    'HANG': [('fix', 'E8w'), ('links',)],
    'DSTR': [('fix', 'E5wbbbb2w'), ('links',)],
    'NPC ': [('fix', 'E8w4hbbh18w'), ('links',)],
    'PLYR': [('fix', 'E'), ('links',)],
    'TIMR': [('fix', 'Bw'), ('links',)],
    'CNTR': [('fix', 'Bhh'), ('links',)],
    'SFX ': [('fix', 'Bhhwwwbbbb3www'), ('links',)],
    'GRUP': [('fix', 'Bhh'), ('arr', 'f[4]', 'w'), ('links',)],
    'PORT': [('fix', 'B3wbbbb'), ('links',)],  # sceneID bytes are swapped by the game (HackSwapIt)
    'CAM ': [('fix', 'B15whh6w6w3wbbbb'), ('links',)],
    'BUTN': [('fix', 'E4wbbbb'), ('fix', 'M'), ('links',)],
    'GUST': [('fix', 'B8w'), ('links',)],
    'COND': [('fix', 'B3w'), ('links',)],
    'LOBM': [('fix', 'B30w'), ('links',)],
    'ENV ': [('fix', 'B14w'), ('links',)],
    'DPAT': [('fix', 'B'), ('links',)],
    'UI  ': [('fix', 'Ew2hw8w'), ('links',)],
    'UIFT': [('fix', 'Ew2hw8whbbwbbbbbbbb4h2h2h'), ('links',)],
    'FOG ': [('fix', 'Bbbbbbbbb4wbbbb'), ('links',)],
    'LITE': [('fix', 'Bbbbbw4w3ww4ww'), ('links',)],
    'CSNM': [('fix', 'B3w15w15w15w'), ('links',)],
    'EGEN': [('fix', 'E3wbbbbww'), ('links',)],
    'SCRP': [('fix', 'Bww'), ('arr', 'f[4]', '8w'), ('links',)],
    'MRKR': [('fix', '3w')],
    'VOLU': [('fix', 'Bwbbbb'), ('custom', 'words_to_links')],
    'MINF': [('custom', 'minf')],
    'PICK': [('fix', '2w'), ('arr', 'f[1]', 'wbbh3w')],
    'TEXT': [('custom', 'text')],
    'ALST': [('rest', 'w')],
    'PRJT': [('fix', 'B15w'), ('links',)],
    'ATBL': [('custom', 'atbl')],
    'PARE': [('fix', 'Bbbbbw7w2w3w3wwbbbbbbbb5wbbbbwbbbb'), ('links',)],
    'PARS': [('custom', 'pars')],
    'SURF': [('fix', 'Bbbbbbbbbw6whhwwhhwwhhwww15w15wbbbb'), ('links',)],
    'MAPR': [('fix', '2w'), ('arr', 'f[1]', '2w')],
    'SNDI': [('fix', '3w'), ('arr', 'f[0] + f[2]', '3whh3w16h7h11hw')],
}


def expand(layout):
    out = []
    for m in re.finditer(r'(\d*)([A-Za-z])', layout):
        n = int(m.group(1) or 1)
        c = m.group(2)
        for _ in range(n):
            if c in MACROS:
                out += expand(MACROS[c])
            else:
                out.append(c)
    return out


SIZES = {'w': 4, 'h': 2, 'b': 1, 'd': 8, 'm': 0x30}


def layout_size(layout):
    return sum(SIZES[c] for c in expand(layout))


# ---------------------------------------------------------------- HIP reading
def hip_assets(d):
    out = []

    def walk(o, end):
        while o + 8 <= end:
            t = d[o:o + 4]
            n = struct.unpack('>I', d[o + 4:o + 8])[0]
            if t == b'AHDR':
                aid, typ, off, sz, plus, fl = struct.unpack('>I4sIIII', d[o + 8:o + 32])
                out.append((aid, typ.decode('latin1'), off, sz))
            elif t in (b'HIPA', b'PACK', b'DICT', b'ATOC', b'LTOC', b'STRM', b'LHDR', b'LDBG'):
                walk(o + 8, o + 8 + n)
            o += 8 + n
    walk(0, len(d))
    return out


def read_fields(data, pos, layout):
    vals = []
    for c in expand(layout):
        n = SIZES[c]
        fmt = {'w': '>I', 'h': '>H', 'b': '>B', 'd': '>Q', 'm': '>B'}[c]
        vals.append(struct.unpack_from(fmt, data, pos)[0] if pos + n <= len(data) else None)
        pos += n
    return vals, pos


def validate(types):
    ids = set()
    files = sorted(glob.glob('game/files/**/*.HIP', recursive=True))
    allassets = []
    for f in files:
        d = open(f, 'rb').read()
        for a in hip_assets(d):
            ids.add(a[0])
            allassets.append((f, d, a))
    stats = collections.defaultdict(collections.Counter)
    examples = {}
    for f, d, (aid, typ, off, sz) in allassets:
        if typ not in TYPES or (types and typ not in types):
            continue
        steps = TYPES[typ]
        data = d[off:off + sz]
        pos = 0
        fields = []
        problem = None
        for st in steps:
            if st[0] == 'fix':
                v, pos = read_fields(data, pos, st[1])
                fields += v
            elif st[0] == 'arr':
                cnt = eval(st[1], {'f': fields})
                if cnt is None or cnt > 100000:
                    problem = 'bad count'
                    break
                pos += cnt * layout_size(st[2])
            elif st[0] == 'links':
                lc = data[5] if len(data) > 5 else 0
                expect = pos + lc * 32
                if expect != sz:
                    problem = 'size %d != fixed %d + links %d' % (sz, pos, lc * 32)
                    break
                for k in range(lc):
                    dst = struct.unpack_from('>I', data, pos + k * 32 + 4)[0]
                    if dst and dst not in ids:
                        problem = 'link target %08X unknown' % dst
                pos = sz
            elif st[0] == 'rest':
                pos = sz
            elif st[0] == 'custom':
                pos = sz
        if problem is None and pos != sz and steps[-1][0] not in ('custom',):
            problem = 'size %d != parsed %d' % (sz, pos)
        if problem is None and steps[0][1][:1] in ('B', 'E') and struct.unpack_from('>I', data, 0)[0] != aid:
            problem = 'id mismatch'
        stats[typ]['ok' if problem is None else 'bad'] += 1
        if problem and typ not in examples:
            examples[typ] = (f, aid, sz, problem, data[:64].hex())
    for t in sorted(stats):
        s = stats[t]
        print('%-5s ok %5d bad %5d' % (t, s['ok'], s['bad']))
        if t in examples:
            e = examples[t]
            print('      e.g. %s %08X size %X: %s\n      %s' % e)


def gen():
    print('// Generated by pc/tools/asset_layouts.py -- do not edit.')
    for t, steps in sorted(TYPES.items()):
        parts = []
        for st in steps:
            if st[0] == 'fix':
                parts.append('{ PC_STEP_FIX, "%s", -1 }' % ''.join(expand(st[1])))
            elif st[0] == 'arr':
                m = re.match(r'f\[(\d+)\]$', st[1])
                if not m:
                    parts.append('{ PC_STEP_CUSTOM, "%s", -1 }' % t.strip().lower())
                    break
                parts.append('{ PC_STEP_ARR, "%s", %s }' % (''.join(expand(st[2])), m.group(1)))
            elif st[0] == 'links':
                parts.append('{ PC_STEP_LINKS, "", -1 }')
            elif st[0] == 'rest':
                parts.append('{ PC_STEP_REST, "%s", -1 }' % ''.join(expand(st[1])))
            elif st[0] == 'custom':
                parts.append('{ PC_STEP_CUSTOM, "%s", -1 }' % st[1])
        parts.append('{ PC_STEP_END, "", -1 }')
        print("PC_ASSET_TYPE('%s', %s)" % (t, ', '.join(parts)))


if __name__ == '__main__':
    if sys.argv[1] == 'validate':
        validate(set(sys.argv[2:]))
    else:
        gen()
