#!/usr/bin/env python3
# Tries source variants of one function: JSON config {file, bak, unit, fn, old, new:[...]}.
# Each "new" replaces "old" in the backup copy, rebuilds via check_unit, prints the score.
# Usage: python tools/variants.py <config.json>
import subprocess, shutil, sys, json
# usage: variants.py <file> <unit> <fn> <bak> <old> <new1> [<new2> ...]  (strings via JSON file)
cfg=json.load(open(sys.argv[1]))
p=cfg['file']; bak=cfg['bak']
s0=open(bak).read()
assert cfg['old'] in s0, 'old not found'
for new in cfg['new']:
    open(p,'w').write(s0.replace(cfg['old'],new))
    out=subprocess.run(['python','tools/check_unit.py','GIHE78',cfg['unit']],capture_output=True,text=True).stdout
    print(repr(new[:90]), [l.strip() for l in out.splitlines() if cfg['fn'] in l])
shutil.copy(bak,p)
