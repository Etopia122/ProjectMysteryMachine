#!/usr/bin/env python3
# Compile all game sources for PC and list the external symbols they need,
# grouped by the library that provided them on GameCube.
#
# usage: python pc/tools/imports.py [--cc path/to/g++]
# Writes pc/survey/imports.txt.
import argparse, collections, concurrent.futures, os, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser()
ap.add_argument('--cc', default='C:/msys64/mingw32/bin/g++.exe')
a = ap.parse_args()
bindir = Path(a.cc).parent
env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ.get('PATH', ''))
objdir = ROOT / 'pc' / 'survey' / 'obj'
objdir.mkdir(parents=True, exist_ok=True)
srcs = sorted(p for d in ('src/Core/x', 'src/Core/gc', 'src/Game')
              for p in (ROOT / d).iterdir() if p.suffix in ('.c', '.cpp'))

def build(p):
    o = objdir / (p.stem + '.o')
    cmd = [a.cc, '-c', '-x', 'c++', '-std=gnu++98', '-fpermissive', '-w', '-O0',
           '-include', str(ROOT / 'pc/include/pc_compat.h'),
           '-I', str(ROOT / 'pc/include'), '-I', str(ROOT / 'include'), '-I', str(ROOT / 'src/Core/x'),
           '-I', str(ROOT / 'src/Core/gc'), '-I', str(ROOT / 'src/Game'),
           '-DPC_PORT=1', '-DVERSION_GIHE78', '-DBUILD_VERSION=0', str(p), '-o', str(o)]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env)
    return p, o if r.returncode == 0 else None, r.stderr

objs = []
with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as ex:
    for p, o, err in ex.map(build, srcs):
        if o: objs.append(o)
        else: print('FAILED', p.name, err[:300])

defined, undefined = set(), collections.defaultdict(set)
for o in objs:
    r = subprocess.run([str(bindir / 'nm.exe'), '-C', str(o)], capture_output=True, text=True, env=env)
    for l in r.stdout.splitlines():
        parts = l.split(None, 2)
        if len(parts) == 3 and parts[1] in 'TDBRCVW':
            defined.add(parts[2])
        elif len(parts) == 2 and parts[0] == 'U':
            undefined[parts[1]].add(o.stem)

need = {s: u for s, u in undefined.items() if s not in defined}

def group(s):
    n = s.split('(')[0]
    if re.match(r'(_?_?rw|_?_?Rw|_?_?Rp|_?_?Rt|_?_?Rx|_rp|_rx|_rt)', n): return 'RenderWare'
    if re.match(r'Bink|RAD', n): return 'Bink'
    if re.match(r'(GX|OS|DVD|PAD|VI|AX|AI|AR|ARQ|DSP|CARD|SI|EXI|DB|PPC|LC|MTX|PSMTX|PSVEC|PSQUAT|C_MTX|C_VEC|VEC|MTX|__OS|__GX|__DVD|__CARD|SP|mix|MIX|SYN|seq)', n): return 'Dolphin SDK'
    if re.match(r'(__cxa|__gxx|_Unwind|operator|std::|__dso|atexit|_?_?mingw|__chk|__security|__stack)', n): return 'C++ runtime'
    return 'C library / other'

groups = collections.defaultdict(list)
for s in sorted(need):
    groups[group(s)].append(s)
lines = ['%d game objects, %d external symbols' % (len(objs), len(need))]
for g in sorted(groups, key=lambda g: -len(groups[g])):
    lines += ['', '== %s (%d)' % (g, len(groups[g]))]
    lines += ['  %-50s %s' % (s, ','.join(sorted(need[s])[:4])) for s in groups[g]]
(ROOT / 'pc' / 'survey' / 'imports.txt').write_text('\n'.join(lines) + '\n')
print('\n'.join(l for l in lines if not l.startswith('  ')))
