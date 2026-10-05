#!/usr/bin/env python3
# Per-unit progress from build/GIHE78/report.json (run `ninja progress` first).
# usage: tools/unitprog.py [unit substring ...]
import json, sys
r = json.load(open('build/GIHE78/report.json'))
for u in r['units']:
    if sys.argv[1:] and not any(a in u['name'] for a in sys.argv[1:]):
        continue
    m = u.get('measures', {})
    fs = u.get('functions', [])
    print('%-32s code %6s matched %6s (%5.1f%%) fuzzy %5.1f%%  funcs %d/%d' % (
        u['name'].split('/')[-1], m.get('total_code', 0), m.get('matched_code', 0), m.get('matched_code_percent', 0),
        m.get('fuzzy_match_percent', 0), sum(1 for f in fs if f.get('fuzzy_match_percent') == 100), len(fs)))
