#!/usr/bin/env python3
# Copy the MinGW/SDL2 DLLs scooby.exe needs next to it, so it runs
# outside the MSYS2 shell (from Explorer or a plain terminal).
# usage: python pc/tools/deploy.py
import os, re, shutil, subprocess
from pathlib import Path

OUT = Path('build/pc')


def find_tool(name):
    exe = name + ('.exe' if os.name == 'nt' else '')
    for prefix in (os.environ.get('MINGW_PREFIX'), 'C:/msys64/mingw32'):
        if prefix:
            candidate = Path(prefix) / 'bin' / exe
            if candidate.exists():
                return candidate
    found = shutil.which(exe) or shutil.which(name)
    if found:
        return Path(found)
    raise SystemExit('%s not found: run this from an MSYS2 mingw32 shell (or put its bin on PATH)' % name)


OBJDUMP = find_tool('objdump')
BIN = OBJDUMP.parent


def imports(path):
    out = subprocess.run([str(OBJDUMP), '-p', str(path)], capture_output=True, text=True).stdout
    return re.findall(r'DLL Name: (\S+)', out)


todo, done = [p for p in (OUT / 'scooby.exe', OUT / 'scooby_launcher.exe') if p.exists()], set()
while todo:
    for name in imports(todo.pop()):
        src = BIN / name
        if name.lower() in done or not src.exists():
            continue  # system DLL
        done.add(name.lower())
        shutil.copy2(src, OUT / name)
        todo.append(src)
# Remove MinGW DLLs an earlier build needed but this one doesn't.
stale = [p for p in OUT.glob('*.dll') if p.name.lower() not in done and (BIN / p.name).exists()]
for p in stale:
    p.unlink()
print('%d DLLs copied to %s, %d stale removed' % (len(done), OUT, len(stale)))
