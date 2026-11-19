#!/usr/bin/env python3
# Puts together the PC release folder build/dist/ProjectMysteryMachine
# (and ProjectMysteryMachine.zip):
# the launcher, the game, SDL2.dll + libwinpthread-1.dll, a read-me and the
# third-party licences.
# No game data: players extract their own disc with the launcher.
# usage: python pc/tools/package.py   (after sh pc/tools/build.sh)
import os, re, shutil, subprocess, zipfile
from pathlib import Path

BUILD = Path(os.environ.get('PC_BUILD_DIR', 'build/pc'))
OUT = Path('build/dist/ProjectMysteryMachine')
FFMPEG_ROOT = Path('build/ffmpeg')
FORBIDDEN = {'.iso', '.gcm', '.rvz', '.wia', '.ciso', '.gcz', '.dol', '.rel', '.hip', '.bik', '.aw', '.txd'}


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


STRIP = find_tool('strip')
OBJDUMP = find_tool('objdump')
BIN = OBJDUMP.parent

README = '''Project: Mystery Machine
========================
Scooby-Doo! Night of 100 Frights - PC port

This is a native Windows version of the GameCube game, built from the
decompiled game code. It contains no game data: you need your own copy of the
game on GameCube (the USA release, disc ID GIHE78) as a disc image (.iso or
.gcm) dumped from your disc.

Fan project
-----------
This is an unofficial, non-commercial fan project. It is not affiliated with,
sponsored by or endorsed by Warner Bros. Discovery, Hanna-Barbera, Cartoon
Network, THQ, Heavy Iron Studios or Nintendo. Scooby-Doo! Night of 100
Frights, Scooby-Doo and all related characters and logos belong to their
respective owners. This archive holds no game data and no part of the game;
you must own the disc to use it.

The launcher currently uses third-party clip art for its Scooby-Doo badge and
Mystery Machine van, which is licensed for personal use only: treat this
archive as a personal fan build, not as something to redistribute.

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
          ratio (widescreen shows more at the sides, with the HUD in the
          corners; the title screen and movies stay 4:3), anti-aliasing,
          vertical sync.
Graphics: draw distance (moves the far edge of the view and the fog back),
          effects distance (how far particle effects show), texture filtering,
          frame rate (30 as on the GameCube, or 60 with the same jumps and
          timings), field of view, brightness, shadow quality.
Controls: every GameCube button for the keyboard (one or more keys) and for a
          controller (any button or trigger), swapping the analog sticks, and
          the debug menu toggle (off by default).
Game:     skipping the company logo movies at start.

Default controls
----------------
Keyboard:   WASD or arrows move, I J K L turn the camera (C-stick),
            Space = A, Shift = B, E = X, R = Y, Enter = Start, Q = L, Tab = R,
            F = Z, number pad 8 4 2 6 = D-pad.
Controller: buttons as labelled, triggers = L and R, right bumper = Z, left
            stick moves, right stick is the camera.
While playing: Alt+Enter or F11 switches fullscreen. With the debug menu
enabled in Controls, F1 (or the controller's Back / View button) opens it and
F3 shows Scooby's position.

Third-party software
--------------------
SDL2 (zlib licence), libwinpthread (mingw-w64 runtime, MIT licence) and FFmpeg
(LGPL 2.1 or later; only its Bink decoders, linked into scooby.exe) - see the
licences folder. FFmpeg's source:
https://ffmpeg.org/releases/ffmpeg-8.1.1.tar.xz, built with
pc/tools/build_ffmpeg.sh from this port's source.
'''


def strip_copy(src, dst):
    shutil.copy2(src, dst)
    subprocess.run([str(STRIP), '--strip-debug', str(dst)], check=True)


def imports(path):
    out = subprocess.run([str(OBJDUMP), '-p', str(path)], capture_output=True, text=True).stdout
    return re.findall(r'DLL Name: (\S+)', out)


def sdl_license():
    for candidate in (BIN.parent / 'share' / 'licenses' / 'SDL2' / 'LICENSE.txt',
                      Path('C:/msys64/mingw32/share/licenses/SDL2/LICENSE.txt')):
        if candidate.exists():
            return candidate
    raise SystemExit('SDL2 licence not found; install mingw-w64-i686-SDL2')


def winpthread_license():
    for candidate in (BIN.parent / 'share' / 'licenses' / 'winpthreads' / 'COPYING',
                      Path('C:/msys64/mingw32/share/licenses/winpthreads/COPYING')):
        if candidate.exists():
            return candidate
    return None


def ffmpeg_source():
    for candidate in sorted(FFMPEG_ROOT.glob('ffmpeg-*')):
        if (candidate / 'COPYING.LGPLv2.1').exists() and (candidate / 'LICENSE.md').exists():
            return candidate
    raise SystemExit('minimal FFmpeg source not found: run sh pc/tools/build_ffmpeg.sh first')


if OUT.exists():
    shutil.rmtree(OUT)
(OUT / 'licences').mkdir(parents=True)
for exe in ('scooby.exe', 'scooby_launcher.exe'):
    if not (BUILD / exe).exists():
        raise SystemExit('%s is missing: build it first (sh pc/tools/build.sh)' % (BUILD / exe))
    strip_copy(BUILD / exe, OUT / exe)
# The MinGW DLLs the programs import (SDL2.dll, plus the launcher's
# libwinpthread-1.dll). Everything else is static.
dlls, todo = set(), [OUT / 'scooby.exe', OUT / 'scooby_launcher.exe']
while todo:
    for name in imports(todo.pop()):
        if name.lower() not in dlls and (BIN / name).exists():
            dlls.add(name.lower())
            shutil.copy2(BIN / name, OUT / name)
            todo.append(BIN / name)
(OUT / 'README.txt').write_text(README.replace('\n', '\r\n'), encoding='utf-8', newline='')
shutil.copy2(sdl_license(), OUT / 'licences' / 'SDL2.txt')
wp = winpthread_license()
if wp and (OUT / 'libwinpthread-1.dll').exists():
    shutil.copy2(wp, OUT / 'licences' / 'libwinpthread.txt')
ffmpeg = ffmpeg_source()
shutil.copy2(ffmpeg / 'COPYING.LGPLv2.1', OUT / 'licences' / 'FFmpeg-LGPL-2.1.txt')
shutil.copy2(ffmpeg / 'LICENSE.md', OUT / 'licences' / 'FFmpeg-LICENSE.md')

# Guard the "no game assets" promise. This is deliberately checked after the
# folder is assembled, before it becomes an archive.
bad = [p for p in OUT.rglob('*') if p.is_file() and p.suffix.lower() in FORBIDDEN]
if bad:
    raise SystemExit('game data leaked into the release: ' + ', '.join(str(p) for p in bad))
if not (OUT / 'scooby_launcher.exe').exists():
    raise SystemExit('the launcher is missing from the release')

archive = OUT.parent / 'ProjectMysteryMachine.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for f in sorted(OUT.rglob('*')):
        if f.is_file():
            z.write(f, Path('ProjectMysteryMachine') / f.relative_to(OUT))
size = sum(f.stat().st_size for f in OUT.rglob('*') if f.is_file())
print('%s: %d files, %.1f MB (%s, %.1f MB); DLLs: %s' % (OUT, sum(1 for f in OUT.rglob('*') if f.is_file()),
      size / 1e6, archive, archive.stat().st_size / 1e6, ', '.join(sorted(dlls))))
