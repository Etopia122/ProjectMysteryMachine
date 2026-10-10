#!/usr/bin/env python3
# Copy the MinGW/SDL2 DLLs scooby.exe needs next to it, so it runs
# outside the MSYS2 shell (from Explorer or a plain terminal).
# usage: python pc/tools/deploy.py
import re, shutil, subprocess
from pathlib import Path

BIN = Path('C:/msys64/mingw32/bin')
OUT = Path('build/pc')

def imports(path):
    out = subprocess.run([str(BIN / 'objdump.exe'), '-p', str(path)], capture_output=True, text=True).stdout
    return re.findall(r'DLL Name: (\S+)', out)

todo, done = [OUT / 'scooby.exe'], set()
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
