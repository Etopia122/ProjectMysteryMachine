# Compare the immediates used in integer compares (cmpwi/cmplwi) and the
# masks tested with rlwinm./andi. per function between our objects and the
# original asm. A wrong enum value or flag bit in a condition shows up here.
# usage: cmpcheck.py [unit substring...]
import collections, os, re, subprocess, sys

cfg = open('configure.py').read()
units = re.findall(r'Object\(\s*Equivalent,\s*"([^"]+)"', cfg)
if sys.argv[1:]:
    units = [u for u in units if any(a in u for a in sys.argv[1:])]


def mask(mb, me):
    m = 0
    for b in range(32):
        inside = (mb <= b <= me) if mb <= me else (b >= mb or b <= me)
        if inside:
            m |= 1 << (31 - b)
    return m


def parse(path):
    out = {}
    cur = None
    for l in open(path, errors='replace'):
        l = re.sub(r'^/\* [0-9A-F]+ [0-9A-F]+  [0-9A-F ]+ \*/\t', '', l).strip()
        m = re.match(r'\.fn (\S+?),', l)
        if m:
            cur = m.group(1)
            out[cur] = set()
            continue
        if l.startswith('.endfn'):
            cur = None
            continue
        if not cur:
            continue
        m = re.match(r'(cmpwi|cmplwi) (?:cr\d, )?r\d+, (-?0x[0-9a-f]+|-?\d+)$', l)
        if m:
            out[cur].add(('cmp', int(m.group(2), 0) & 0xFFFFFFFF))
            continue
        m = re.match(r'rlwinm\. r\d+, r\d+, 0, (\d+), (\d+)$', l)
        if m:
            out[cur].add(('bit', mask(int(m.group(1)), int(m.group(2)))))
            continue
        m = re.match(r'clrlwi\. r\d+, r\d+, (\d+)$', l)
        if m:
            out[cur].add(('bit', mask(int(m.group(1)), 31)))
            continue
        m = re.match(r'andi\. r\d+, r\d+, (0x[0-9a-f]+|\d+)$', l)
        if m:
            out[cur].add(('bit', int(m.group(1), 0)))
    return out


tot = 0
for u in units:
    base = os.path.splitext(u)[0]
    orig = 'build/GIHE78/asm/%s.s' % base
    obj = 'build/GIHE78/src/%s.o' % base
    if not (os.path.exists(orig) and os.path.exists(obj)):
        continue
    ours = 'build/cmpcheck_ours.s'
    subprocess.run([os.path.abspath('build/tools/dtk.exe'), 'elf', 'disasm', obj, ours], capture_output=True)
    fo, fu = parse(orig), parse(ours)
    for name, a in fo.items():
        b = fu.get(name)
        if b is None or a == b:
            continue
        # switch lowering turns compares into jump tables and back; only
        # report values that appear on one side as neither a compare nor a bit
        only_a = {x for x in a - b}
        only_b = {x for x in b - a}
        if not only_a and not only_b:
            continue
        tot += 1
        fmt = lambda s: ' '.join('%s:%#x' % x for x in sorted(s))
        print('%s %s\n   orig only: %s\n   ours only: %s' % (base, name, fmt(only_a), fmt(only_b)))
print('%d functions differ' % tot)
