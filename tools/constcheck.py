# Compare the floating-point constants each function loads (lfs/lfd from
# small-data or absolute addresses) between our compiled objects and the
# original asm, by value. Catches mistyped literals in decompiled code.
# usage: constcheck.py [unit substring...]
import os, re, struct, subprocess, sys

cfg = open('configure.py').read()
units = re.findall(r'Object\(\s*Equivalent,\s*"([^"]+)"', cfg)
if sys.argv[1:]:
    units = [u for u in units if any(a in u for a in sys.argv[1:])]

LOAD = re.compile(r'^\s*(lfs|lfd)\s+f\d+,\s*("[^"]+"|[\w.$@]+)(?:\+0x([0-9a-fA-F]+))?@(?:sda21|l)\(')


def parse(path):
    objs, funcs = {}, {}
    cur_obj = cur_fn = None
    for l in open(path, errors='replace'):
        l = re.sub(r'^/\* [0-9A-F]+ [0-9A-F]+  [0-9A-F ]+ \*/\t', '', l)
        m = re.match(r'\.obj ("[^"]+"|\S+?),', l)
        if m:
            cur_obj = m.group(1).strip('"')
            objs[cur_obj] = b''
            continue
        if l.startswith('.endobj'):
            cur_obj = None
            continue
        if cur_obj is not None:
            m = re.match(r'\s*\.4byte 0x([0-9A-Fa-f]+)', l)
            if m:
                objs[cur_obj] += struct.pack('>I', int(m.group(1), 16))
            m = re.match(r'\s*\.float (\S+)', l)
            if m:
                objs[cur_obj] += struct.pack('>f', float(m.group(1)))
            m = re.match(r'\s*\.double (\S+)', l)
            if m:
                objs[cur_obj] += struct.pack('>d', float(m.group(1)))
            continue
        m = re.match(r'\.fn (\S+?),', l)
        if m:
            cur_fn = m.group(1)
            funcs[cur_fn] = []
            continue
        if l.startswith('.endfn'):
            cur_fn = None
            continue
        if cur_fn:
            m = LOAD.match(l)
            if m:
                funcs[cur_fn].append((m.group(1), m.group(2).strip('"'), int(m.group(3) or '0', 16)))
    return objs, funcs


def values(objs, loads):
    out = set()
    for op, name, off in loads:
        b = objs.get(name)
        if b is None or len(b) < off + (8 if op == 'lfd' else 4):
            continue
        v = struct.unpack('>d' if op == 'lfd' else '>f', b[off:off + (8 if op == 'lfd' else 4)])[0]
        out.add(round(v, 6))
    return out


# original constants live in the unit's own asm; shared ones are in other files
allorig = {}
for root, _, files in os.walk('build/GIHE78/asm'):
    for f in files:
        if f.endswith('.s'):
            allorig.update(parse(os.path.join(root, f))[0])

tot = 0
for u in units:
    base = os.path.splitext(u)[0]
    orig = 'build/GIHE78/asm/%s.s' % base
    obj = 'build/GIHE78/src/%s.o' % base
    if not (os.path.exists(orig) and os.path.exists(obj)):
        continue
    ours = 'build/constcheck_ours.s'
    subprocess.run([os.path.abspath('build/tools/dtk.exe'), 'elf', 'disasm', obj, ours], capture_output=True)
    _, fo = parse(orig)
    uo, fu = parse(ours)
    for name, loads in fo.items():
        if name not in fu:
            continue
        a = values(allorig, loads)
        b = values(uo, fu[name])
        if a != b and b:
            tot += 1
            print('%s %s\n   orig only: %s\n   ours only: %s' % (base, name, sorted(a - b), sorted(b - a)))
print('%d functions differ' % tot)
