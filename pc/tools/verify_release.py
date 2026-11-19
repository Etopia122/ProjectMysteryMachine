#!/usr/bin/env python3
# Check a packaged PC release: the launcher must be present, the game must be
# present, and no game assets or disc images may be inside.
# usage: python pc/tools/verify_release.py [build/dist/ProjectMysteryMachine.zip]
import sys, zipfile
from pathlib import Path

archive = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/dist/ProjectMysteryMachine.zip')
if not archive.exists():
    raise SystemExit('%s does not exist' % archive)

required = {
    'ProjectMysteryMachine/scooby.exe',
    'ProjectMysteryMachine/scooby_launcher.exe',
    'ProjectMysteryMachine/SDL2.dll',
    'ProjectMysteryMachine/libwinpthread-1.dll',
    'ProjectMysteryMachine/README.txt',
}
forbidden_suffixes = {'.iso', '.gcm', '.rvz', '.wia', '.ciso', '.gcz', '.dol', '.rel', '.hip', '.bik', '.aw', '.txd'}
forbidden_roots = ('ProjectMysteryMachine/data/', 'ProjectMysteryMachine/game/', 'ProjectMysteryMachine/orig/')

with zipfile.ZipFile(archive) as z:
    names = set(z.namelist())

missing = sorted(required - names)
if missing:
    raise SystemExit('release is missing: ' + ', '.join(missing))
# The console build is for testing only.
debug_exes = sorted(n for n in names if '_debug' in n.lower())
if debug_exes:
    raise SystemExit('release contains a debug build: ' + ', '.join(debug_exes))
bad = sorted(n for n in names
             if n.lower().endswith(tuple(forbidden_suffixes)) or n.startswith(forbidden_roots))
if bad:
    raise SystemExit('release contains game data: ' + ', '.join(bad))

print('%s: launcher present, no game assets, %d files' % (archive, len(names)))
