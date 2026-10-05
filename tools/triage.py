#!/usr/bin/env python3
# Classify every not-yet-matching function (from build/GIHE78/report.json) by
# what its diff against the original consists of:
#   reg   - only register numbers differ
#   data  - only relocation offsets/addresses differ (data layout)
#   sched - same instruction multiset, different order
#   code  - instructions differ
# usage: tools/triage.py [min fuzzy] [unit substrings...]
import collections
import json
import re
import subprocess
import sys

minf = float(sys.argv[1]) if len(sys.argv) > 1 else 95.0
units = sys.argv[2:]
r = json.load(open('build/GIHE78/report.json'))
srcmap = {}
for u in json.load(open('objdiff.json'))['units']:
    meta = u.get('metadata', {})
    if meta.get('source_path'):
        srcmap[u['name']] = meta['source_path']

REG = re.compile(r'\b[rf]\d+\b|\bcr\d\b')


def norm(s, regs=True, offs=True):
    s = s.strip()
    s = re.sub(r'(b|bl|beq|bne|blt|bge|ble|bgt|bdnz|bdz)\s+0x[0-9a-f]+', r'\1 L', s)
    if regs:
        s = REG.sub('R', s)
    if offs:
        s = re.sub(r'-?0x[0-9a-f]+\((r2|r13|R)\)', 'X(B)', s) if False else s
    return s


rows = []
for u in r['units']:
    if units and not any(x in u['name'] for x in units):
        continue
    src = srcmap.get(u['name'])
    if not src:
        continue
    for f in u.get('functions', []):
        fz = f.get('fuzzy_match_percent', 0)
        if fz >= 100 or fz < minf:
            continue
        out = subprocess.run(['sh', 'tools/diff_fn.sh', src, f['name']], capture_output=True, text=True).stdout
        a, b = [], []
        for l in out.splitlines():
            if '|' not in l:
                continue
            x, y = l[3:].split('|', 1)
            if x.strip():
                a.append(x.strip())
            if y.strip():
                b.append(y.strip())
        if not a:
            continue
        na = [norm(x) for x in a]
        nb = [norm(y) for y in b]
        if [norm(x, False) for x in a] == [norm(y, False) for y in b]:
            kind = 'data'
        elif na == nb:
            kind = 'reg'
        else:
            # ignore relocation-target values: ours shows 0 / 0(0) for unresolved
            def strip(s):
                s = re.sub(r'-?0x[0-9a-f]+\(R\)', 'M(R)', s)
                s = re.sub(r'\b0\(0\)', 'M(R)', s)
                s = re.sub(r'(addi|lis|li|ori) R, (R, )?-?(0x)?[0-9a-f]+$', r'\1 R, K', s)
                return s
            sa, sb = [strip(x) for x in na], [strip(y) for y in nb]
            if sa == sb:
                kind = 'reg+data'
            elif collections.Counter(sa) == collections.Counter(sb):
                kind = 'sched'
            else:
                kind = 'code'
        rows.append((int(f['size']), fz, kind, u['name'].split('/')[-1], f['name']))
        print('%-8s %6d %6.2f %-14s %s' % (kind, int(f['size']), fz, u['name'].split('/')[-1], f['name'][:60]), flush=True)
