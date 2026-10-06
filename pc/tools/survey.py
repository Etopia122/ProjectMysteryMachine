#!/usr/bin/env python3
# Portability survey: syntax-check every game source with a PC compiler and
# group the first errors of each file into categories.
#
# usage: python pc/tools/survey.py [--cc path/to/g++] [--m32] [file filter...]
# Writes pc/survey/<unit>.log per file and pc/survey/summary.txt.
import argparse, collections, concurrent.futures, os, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser()
ap.add_argument('--cc', default='C:/msys64/mingw32/bin/g++.exe')
ap.add_argument('--m32', action='store_true')
ap.add_argument('filters', nargs='*')
a = ap.parse_args()

srcs = sorted(p for d in ('src/Core/x', 'src/Core/gc', 'src/Game')
              for p in (ROOT / d).iterdir() if p.suffix in ('.c', '.cpp'))
if a.filters:
    srcs = [p for p in srcs if any(f in str(p) for f in a.filters)]
out = ROOT / 'pc' / 'survey'
out.mkdir(parents=True, exist_ok=True)

def run(p):
    lang = 'c++'  # the game builds .c files as C++ too (-lang=c++)
    cmd = [a.cc, '-fsyntax-only', '-x', lang, '-std=gnu++98' if lang == 'c++' else '-std=gnu99',
           '-fpermissive', '-w', '-ferror-limit=0' if 'clang' in a.cc else '-fmax-errors=0',
           '-include', str(ROOT / 'pc/include/pc_compat.h'),
           '-I', str(ROOT / 'pc/include'), '-I', str(ROOT / 'include'), '-I', str(ROOT / 'src/Core/x'),
           '-I', str(ROOT / 'src/Core/gc'), '-I', str(ROOT / 'src/Game'),
           '-DPC_PORT=1', '-DVERSION_GIHE78', '-DBUILD_VERSION=0']
    if a.m32:
        cmd.append('-m32')
    env = dict(os.environ, PATH=str(Path(a.cc).parent) + os.pathsep + os.environ.get('PATH', ''))
    r = subprocess.run(cmd + [str(p)], capture_output=True, text=True, env=env)
    (out / (p.stem + '.log')).write_text(r.stderr)
    errs = [l for l in r.stderr.splitlines() if ': error:' in l]
    if r.returncode and not errs:
        errs = ['%s: error: compiler failed (%d)' % (p.name, r.returncode)]
    return p, errs

def category(msg):
    m = msg.split(': error: ', 1)[1]
    m = re.sub(r"'[^']*'", "'X'", m)
    m = re.sub(r'\d+', 'N', m)
    return m[:90]

cats = collections.Counter()
files = []
with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as ex:
    for p, errs in ex.map(run, srcs):
        files.append((len(errs), p.relative_to(ROOT).as_posix()))
        for e in errs:
            cats[category(e)] += 1

lines = ['%d/%d files compile cleanly' % (sum(1 for n, _ in files if n == 0), len(files)), '',
         'errors per file:'] + ['%5d  %s' % f for f in sorted(files, reverse=True) if f[0]] + \
        ['', 'error categories:'] + ['%5d  %s' % (n, c) for c, n in cats.most_common(60)]
(out / 'summary.txt').write_text('\n'.join(lines) + '\n')
print('\n'.join(lines[:40]))
