# Compare each function's memory accesses (load/store opcode + displacement,
# ignoring registers and order) between our compiled objects and the original
# asm. Field-offset and pointer-indirection bugs show up as differences that
# register allocation alone can't explain.
# usage: memcheck.py [unit substring...]
import collections, os, re, subprocess, sys

cfg = open('configure.py').read()
units = re.findall(r'Object\(\s*Equivalent,\s*"([^"]+)"', cfg)
if sys.argv[1:]:
    units = [u for u in units if any(a in u for a in sys.argv[1:])]

MEM = re.compile(r'^\s*(l[bhwf][a-z]*u?|st[bhwf][a-z]*u?|lmw|stmw|psq_l|psq_st)\s+[^,]+,\s*(-?0x[0-9a-f]+|-?\d+|[\w.@$"+-]+@sda21)\(r(\d+)\)')


def funcs(path):
    out = {}
    cur = None
    for l in open(path, errors='replace'):
        l = re.sub(r'^/\* [0-9A-F]+ [0-9A-F]+  [0-9A-F ]+ \*/\t', '', l)
        m = re.match(r'\.fn (\S+?),', l)
        if m:
            cur = m.group(1)
            out[cur] = collections.Counter()
            continue
        if l.startswith('.endfn'):
            cur = None
            continue
        if cur:
            m = MEM.match(l)
            if m:
                op, disp, base = m.groups()
                if base == '1':          # stack slots move freely
                    continue
                if '@sda21' in disp:     # small-data symbol names differ
                    disp = 'sda'
                out[cur][(op, disp)] += 1
    return out


tot = 0
for u in units:
    base = os.path.splitext(u)[0]
    orig = 'build/GIHE78/asm/%s.s' % base
    obj = 'build/GIHE78/src/%s.o' % base
    if not (os.path.exists(orig) and os.path.exists(obj)):
        continue
    ours = 'build/memcheck_ours.s'
    subprocess.run([os.path.abspath('build/tools/dtk.exe'), 'elf', 'disasm', obj, ours], capture_output=True)
    fo, fu = funcs(orig), funcs(ours)
    for name, oc in fo.items():
        uc = fu.get(name)
        if uc is None or uc == oc:
            continue
        # ignore pure count changes of the same access (CSE/reloads)
        if set(oc) == set(uc):
            continue
        only_o = sorted(set(oc) - set(uc))
        only_u = sorted(set(uc) - set(oc))
        tot += 1
        print('%s %s\n   orig only: %s\n   ours only: %s' % (base, name, only_o[:8], only_u[:8]))
print('%d functions differ' % tot)
