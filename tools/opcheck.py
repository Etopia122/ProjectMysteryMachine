# Compare per-function histograms of arithmetic/compare/branch-shape opcodes
# between our objects and the original asm. Register allocation and
# scheduling don't change these counts; a different formula or missing
# condition usually does.
# usage: opcheck.py [--min N] [unit substring...]
import collections, os, re, subprocess, sys

args = sys.argv[1:]
minpct = 0
if args[:1] == ['--min']:
    minpct = float(args[1])
    args = args[2:]
cfg = open('configure.py').read()
units = re.findall(r'Object\(\s*Equivalent,\s*"([^"]+)"', cfg)
if args:
    units = [u for u in units if any(a in u for a in args)]

KEEP = re.compile(r'^(f(add|sub|mul|div|madd|msub|nmadd|nmsub|neg|abs|nabs|res|rsqrte|sel|ctiwz?)s?|'
                  r'add[ic]?|subf?[ic]?|mul(li|lw|hwu?)|divwu?|and[ci]?|or[ci]?|xor[i]?|nor|neg|'
                  r'slw|srw|sraw[i]?|rlwinm|rlwimi|cntlzw|extsb|extsh|cmp[lw]*[i]?|fcmp[ou]|cror|bl)$')


def hist(path):
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
            op = l.split()[0] if l.split() else ''
            op = op.rstrip('.')
            if op in ('addi', 'addis', 'li', 'lis', 'mr', 'subi'):
                continue            # address arithmetic and moves vary freely
            if KEEP.match(op):
                out[cur][op] += 1
    return out


FUSED = {'fmadd': ('fmul', 'fadd'), 'fmsub': ('fmul', 'fsub'), 'fnmadd': ('fmul', 'fadd'),
         'fnmsub': ('fmul', 'fsub')}


def norm(h):
    """Split fused multiply-adds, merge single/double forms and add/sub, drop
    negations (contraction and operand order are compiler choices)."""
    out = collections.Counter()
    for k, v in h.items():
        base = k[:-1] if k.endswith('s') and k not in ('fabs', 'fnabs', 'fres') else k
        if base in FUSED:
            for p in FUSED[base]:
                out[p] += v
        elif base in ('fneg', 'fabs', 'fnabs'):
            out['fabs' if base != 'fneg' else 'fneg'] += 0 if base == 'fneg' else v
        else:
            out[base] += v
    for k in ('fadd', 'fsub'):
        out['faddsub'] += out.pop(k, 0)
    return {k: v for k, v in out.items() if v}


tot = 0
for u in units:
    base = os.path.splitext(u)[0]
    orig = 'build/GIHE78/asm/%s.s' % base
    obj = 'build/GIHE78/src/%s.o' % base
    if not (os.path.exists(orig) and os.path.exists(obj)):
        continue
    ours = 'build/opcheck_ours.s'
    subprocess.run([os.path.abspath('build/tools/dtk.exe'), 'elf', 'disasm', obj, ours], capture_output=True)
    ho, hu = hist(orig), hist(ours)
    for name, oc in ho.items():
        uc = hu.get(name)
        if uc is None:
            continue
        # floating-point and compare ops only: integer ops shift with addressing
        fo = norm({k: v for k, v in oc.items() if k.startswith('f') or k in ('cror',)})
        fu = norm({k: v for k, v in uc.items() if k.startswith('f') or k in ('cror',)})
        if fo != fu:
            d = {k: fu.get(k, 0) - fo.get(k, 0) for k in set(fo) | set(fu) if fu.get(k, 0) != fo.get(k, 0)}
            tot += 1
            print('%s %s %s' % (base, name, ' '.join('%s%+d' % kv for kv in sorted(d.items()))))
print('%d functions differ' % tot)
