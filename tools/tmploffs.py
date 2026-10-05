#!/usr/bin/env python3
# For each function of a unit, pair up the base-register offsets used to address
# .rodata (addi rX, rBase, OFF / lwz rX, OFF(rBase) with OFF >= min) on our side
# and the original side of the aligned diff. Prints functions whose offsets
# differ, to locate template/literal layout differences.
# usage: tools/tmploffs.py <src> <min offset> [function substrings...]
import json
import re
import subprocess
import sys

src, lo = sys.argv[1], int(sys.argv[2], 0)
want = sys.argv[3:]
unit = None
for u in json.load(open('objdiff.json'))['units']:
    if u.get('metadata', {}).get('source_path') == src:
        unit = u['name']
r = json.load(open('build/GIHE78/report.json'))
fns = []
for u in r['units']:
    if u['name'] == unit:
        fns = [f['name'] for f in u['functions']]
PAT = re.compile(r'(?:addi r\d+, r\d+, |l[wfh]zu? [rf]\d+, |stw [rf]\d+, )(-?0x[0-9a-f]+)(?:\(r\d+\))?$')


def offs(side):
    out = []
    for s in side:
        m = PAT.search(s.strip())
        if m:
            v = int(m.group(1), 16)
            if v >= lo:
                out.append(v)
    return out


for fn in fns:
    if want and not any(w in fn for w in want):
        continue
    d = subprocess.run(['sh', 'tools/diff_fn.sh', src, fn], capture_output=True, text=True).stdout
    a, b = [], []
    for l in d.splitlines():
        if '|' not in l:
            continue
        x, y = l[3:].split('|', 1)
        a.append(x)
        b.append(y)
    oa, ob = offs(a), offs(b)
    if oa != ob and (oa or ob):
        print('%-44s ours %s' % (fn[:44], ' '.join('%X' % v for v in oa)))
        print('%-44s orig %s' % ('', ' '.join('%X' % v for v in ob)), flush=True)
