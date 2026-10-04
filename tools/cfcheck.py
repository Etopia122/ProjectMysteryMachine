# Compare the control-flow shape of each function between our objects and the
# original asm: loops (backward branches), conditional branches and size.
# Loop counts rarely change with codegen, so a mismatch there usually means a
# missing or extra loop; a large drop in branches or size suggests a missing
# condition or code path. Inlining differences also show up here (a call that
# the original inlined), so check the hits by hand.
# usage: cfcheck.py [--all] [unit substring...]
import os, re, subprocess, sys

args = [a for a in sys.argv[1:] if not a.startswith('--')]
show_all = '--all' in sys.argv
cfg = open('configure.py').read()
units = re.findall(r'Object\(\s*(?:Equivalent|Matching),\s*"([^"]+\.(?:c|cpp))"', cfg)
units = [u for u in units if u.startswith(('Core/', 'Game/'))]
if args:
    units = [u for u in units if any(a in u for a in args)]

COND = re.compile(r'^b(eq|ne|lt|le|gt|ge|ns|so|dnz|dz)(lr)?[+-]?\s')


def parse(path):
    out = {}
    cur = None
    for l in open(path, errors='replace'):
        m = re.match(r'^/\* ([0-9A-F]+) [0-9A-F]+  [0-9A-F ]+ \*/\t(.*)$', l)
        if m:
            addr, ins = int(m.group(1), 16), m.group(2).strip()
        else:
            ins = l.strip()
            addr = None
        m = re.match(r'\.fn (\S+?),', ins)
        if m:
            cur = m.group(1)
            out[cur] = {'loops': 0, 'cond': 0, 'size': 0, 'start': None}
            continue
        if ins.startswith('.endfn'):
            cur = None
            continue
        if not cur or not ins or ins.startswith(('.', '#')) or ins.endswith(':'):
            continue
        f = out[cur]
        f['size'] += 4
        if addr is not None and f['start'] is None:
            f['start'] = addr
        cond = bool(COND.match(ins + ' '))
        if cond:
            f['cond'] += 1
        m = re.match(r'^b\w*\s+(?:cr\d, )?(?:0x([0-9a-f]+)|\.L_([0-9A-F]+)|[\w.]+\+?)', ins)
        if m and cond:
            tgt = None
            if m.group(1) is not None:
                tgt = int(m.group(1), 16)
            elif m.group(2) is not None:
                tgt = int(m.group(2), 16)
            here = addr if addr is not None else (f['start'] or 0) + f['size'] - 4
            if tgt is not None and tgt <= here and (addr is not None or tgt < 0x80000000):
                f['loops'] += 1
    return out


def parse_ours(obj):
    # dtk prints our object's branch targets as .L_<offset> labels relative to
    # the section, with instruction comments carrying section offsets.
    ours = 'build/cfcheck_ours.s'
    subprocess.run([os.path.abspath('build/tools/dtk.exe'), 'elf', 'disasm', obj, ours], capture_output=True)
    return parse(ours)


hits = []
for u in units:
    base = os.path.splitext(u)[0]
    orig = 'build/GIHE78/asm/%s.s' % base
    obj = 'build/GIHE78/src/%s.o' % base
    if not (os.path.exists(orig) and os.path.exists(obj)):
        continue
    fo, fu = parse(orig), parse_ours(obj)
    for name, a in fo.items():
        b = fu.get(name)
        if b is None:
            continue
        dl = b['loops'] - a['loops']
        dc = b['cond'] - a['cond']
        ratio = b['size'] / a['size'] if a['size'] else 1.0
        score = abs(dl) * 10 + max(0, -dc) + (10 if ratio < 0.8 else 0)
        if dl or dc < -2 or ratio < 0.8 or show_all:
            hits.append((score, base, name, a, b, ratio))
hits.sort(key=lambda h: -h[0])
for score, base, name, a, b, ratio in hits:
    print('%-22s %-60s loops %d->%d  cond %3d->%3d  size %5d->%5d (%.2f)' % (
        base.split('/')[-1], name[:60], a['loops'], b['loops'], a['cond'], b['cond'], a['size'], b['size'], ratio))
print('%d functions flagged' % len(hits))
