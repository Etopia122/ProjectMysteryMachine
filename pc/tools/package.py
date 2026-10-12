#!/usr/bin/env python3
# Puts together the PC release folder build/dist/ScoobyPC (and ScoobyPC.zip):
# the launcher, the game, SDL2.dll, a read-me and the third-party licences.
# No game data: players extract their own disc with the launcher.
# usage: python pc/tools/package.py   (after sh pc/tools/rebuild.sh with PC_EXE_NAME=scooby)
import re, shutil, subprocess, zipfile
from pathlib import Path

BIN = Path('C:/msys64/mingw32/bin')
BUILD = Path('build/pc')
OUT = Path('build/dist/ScoobyPC')
FFMPEG = Path('build/ffmpeg/ffmpeg-8.1.1')

README = '''Scooby-Doo! Night of 100 Frights - PC port
==========================================

This is a native Windows version of the GameCube game, built from the
decompiled game code. It contains no game data: you need your own copy of the
game on GameCube (the USA release, disc ID GIHE78) as a disc image (.iso or
.gcm) dumped from your disc.

Getting started
---------------
1. Run scooby_launcher.exe.
2. On the "Game files" page, choose your disc image and press "Extract game
   files". The game's files (about 1.1 GB) are copied into the "data" folder
   here. You only do this once. Compressed images (RVZ, WIA, CISO, GCZ) have
   to be converted to a plain ISO first, e.g. with Dolphin's Convert File.
3. Pick your display, graphics and control settings, then press Play.

The settings are kept in scooby.ini; scooby.exe can also be started directly.
Your saves go in the "save" folder here.

Settings
--------
Display:  resolution, windowed / borderless / exclusive fullscreen, aspect
          ratio (widescreen shows more at the sides in levels; the title,
          menus and movies stay 4:3), anti-aliasing, vertical sync.
Graphics: draw distance (moves the far edge of the view and the fog back),
          effects distance (how far particle effects show), texture filtering.
Controls: every GameCube button for the keyboard (one or more keys) and for a
          controller (any button or trigger), and swapping the analog sticks.

Default controls
----------------
Keyboard:   WASD or arrows move, I J K L turn the camera (C-stick),
            Space = A, Shift = B, E = X, R = Y, Enter = Start, Q = L, Tab = R,
            F = Z, number pad 8 4 2 6 = D-pad.
Controller: buttons as labelled, triggers = L and R, right bumper = Z, left
            stick moves, right stick is the camera.
While playing: Alt+Enter or F11 switches fullscreen; F1 (or the controller's
Back / View button) opens the debug menu; F3 shows Scooby's position.

Third-party software
--------------------
SDL2 (zlib licence) and FFmpeg (LGPL 2.1 or later; only its Bink decoders,
linked into scooby.exe) - see the licences folder. FFmpeg's source:
https://ffmpeg.org/releases/ffmpeg-8.1.1.tar.xz, built with
pc/tools/build_ffmpeg.sh from this port's source.
'''


def strip_copy(src, dst):
    shutil.copy2(src, dst)
    subprocess.run([str(BIN / 'strip.exe'), '--strip-debug', str(dst)], check=True)


def imports(path):
    out = subprocess.run([str(BIN / 'objdump.exe'), '-p', str(path)], capture_output=True, text=True).stdout
    return re.findall(r'DLL Name: (\S+)', out)


if OUT.exists():
    shutil.rmtree(OUT)
(OUT / 'licences').mkdir(parents=True)
for exe in ('scooby.exe', 'scooby_launcher.exe'):
    if not (BUILD / exe).exists():
        raise SystemExit('%s is missing: build it first (PC_EXE_NAME=scooby)' % (BUILD / exe))
    strip_copy(BUILD / exe, OUT / exe)
# The MinGW DLLs both programs import (SDL2.dll; everything else is static).
dlls, todo = set(), [OUT / 'scooby.exe', OUT / 'scooby_launcher.exe']
while todo:
    for name in imports(todo.pop()):
        if name.lower() not in dlls and (BIN / name).exists():
            dlls.add(name.lower())
            shutil.copy2(BIN / name, OUT / name)
            todo.append(BIN / name)
(OUT / 'README.txt').write_text(README.replace('\n', '\r\n'), encoding='utf-8', newline='')
shutil.copy2(Path('C:/msys64/mingw32/share/licenses/SDL2/LICENSE.txt'), OUT / 'licences' / 'SDL2.txt')
shutil.copy2(FFMPEG / 'COPYING.LGPLv2.1', OUT / 'licences' / 'FFmpeg-LGPL-2.1.txt')
shutil.copy2(FFMPEG / 'LICENSE.md', OUT / 'licences' / 'FFmpeg-LICENSE.md')

archive = OUT.parent / 'ScoobyPC.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for f in sorted(OUT.rglob('*')):
        if f.is_file():
            z.write(f, Path('ScoobyPC') / f.relative_to(OUT))
size = sum(f.stat().st_size for f in OUT.rglob('*') if f.is_file())
print('%s: %d files, %.1f MB (%s, %.1f MB); DLLs: %s' % (OUT, sum(1 for f in OUT.rglob('*') if f.is_file()),
      size / 1e6, archive, archive.stat().st_size / 1e6, ', '.join(sorted(dlls))))
