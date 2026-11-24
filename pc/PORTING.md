# Project: Mystery Machine — Scooby-Doo! Night of 100 Frights PC port

This branch (`pc-port`) is a fork of the GameCube decompilation. The decomp keeps
the byte-matching GameCube build; this fork turns the same game code into a
native PC program. Game assets are not included: the player supplies their own disc.

## Where things stand (survey, 2026-10-06)

`python pc/tools/survey.py` syntax-checks every game source with GCC;
`python pc/tools/imports.py` compiles them and lists what they need from outside.

- All 133 game sources (`src/Core/x`, `src/Core/gc`, `src/Game`) compile with GCC
  after three small fixes (a C++ case-scope error, `main` renamed to
  `scooby_main`, `.c` files built as C++ like the original).
- The game code has no inline assembly and includes no SDK headers. It reaches
  the outside world through 372 external symbols (`pc/survey/imports.txt`):

| Group | Symbols | PC replacement |
|---|---|---|
| RenderWare 3.x (Rw/Rp/Rt) | 121 | librw (MIT), plus a thin C-API shim |
| Dolphin SDK (GX, OS, DVD, PAD, VI, AX/AI/AR, CARD) | 126 | rewrite the `src/Core/gc` platform layer (`i*.cpp`) on SDL2 |
| C library / leftovers | 109 | libc covers most; ~50 `fn_`/`lbl_` placeholders still point at unsplit asm and must be identified |
| Bink | 11 | FFmpeg's Bink decoder, or skip videos at first |
| C++ runtime | 5 | toolchain |

## Plan

1. **Toolchain (installed).** Build 32-bit first (asset structs and RenderWare
   data assume 4-byte pointers): MSYS2 `mingw-w64-i686-gcc`, `-cmake`, `-SDL2`
   in `C:/msys64/mingw32`.
   64-bit comes later, once pointer fields in asset layouts are handled.
2. **Name the leftovers.** Resolve the `fn_*`/`lbl_*` placeholders to the real
   SDK/RW/game functions and data they stand for (the decomp's symbols.txt and asm).
3. **Link with stubs (done).** `sh pc/tools/rebuild.sh` builds the game library
   with CMake, generates `pc/src/stubs_gen.S` (`pc/tools/genstubs.py`) for every
   symbol nothing on PC provides yet, and links `build/pc/scooby.exe`. Each stub
   prints its name the first time it runs. Real implementations go in `pc/src/`;
   rerunning the script drops their stubs.
4. **Assets and endianness (decided: GameCube disc).** HIP archives and their
   contents are big-endian on GameCube, and textures and geometry are
   GameCube-native RenderWare data. The port byte-swaps game data and converts
   RenderWare data to librw's formats at load time.
5. **Platform layer.** Replace `src/Core/gc/i*.cpp` piece by piece:
   files (DVD -> stdio), memory (OS heap -> malloc), time, pad (SDL gamepad),
   rendering (RenderWare via librw on OpenGL), sound (AX -> SDL audio, DSP-ADPCM
   decoder), saves (CARD -> files), FMV (Bink -> FFmpeg or skip).
   Done so far (2026-10-06): OS heap, ARAM, interrupts, DVD, VI/time, pad,
   our own RenderWare 3.x layer on OpenGL (`pc/rw`), asset byte swapping,
   audio (`audio.cpp`: AX voices, DSP-ADPCM, MIX, SDL output) and FMV
   (`fmv.cpp`: Bink via FFmpeg, replacing `iFMV.cpp` and `iRad3D.c`). The GX
   calls the game makes directly (colour/alpha write masks for depth-only
   passes, the env FX alpha compare) are handled in `pc/rw/pc_gl.cpp`. Boots through the logo
   movies and Intro to the title screen and main menu; New Game loads the
   first level, where Scooby can run around; memory card saves go to files
   (`card.cpp`).
6. **Boot to the title screen**, then into a level, then fix gameplay bugs by
   comparing with the GameCube build (Dolphin + the decomp's debugging tools).

## Running

```
sh pc/tools/build_ffmpeg.sh     # once: minimal static FFmpeg (Bink only) in build/ffmpeg
sh pc/tools/rebuild.sh          # build
python pc/tools/deploy.py       # copy SDL2.dll/libwinpthread-1.dll next to the exes
build/pc/scooby_launcher.exe    # or build/pc/scooby.exe; data from SCOOBY_DATA, scooby.ini or game/files
```

`build_ffmpeg.sh` downloads the FFmpeg source release from ffmpeg.org, checks
its signature and builds only the Bink demuxer and decoders (LGPL, no external
libraries; needs `pacman -S make diffutils`). It is linked statically, as are
libgcc and libstdc++, so SDL2.dll and the launcher's small
libwinpthread-1.dll runtime are the only DLLs that ship. Without the static
FFmpeg, CMake falls back to MSYS2's shared FFmpeg (~70 DLLs).

### Launcher, settings and release folder

`scooby_launcher.exe` (pc/launcher, built with the game) is how players start:
it extracts the game's files from their own disc image (USA, GIHE78; plain
.iso/.gcm) into `data/` beside the program, edits `scooby.ini` and starts
scooby.exe. `scooby_launcher --extract <image> [folder]` extracts without the
window, printing `progress 0.42 ...` lines to stdout (or the terminal it was
started from) for a caller that shows progress. The themed launcher uses a sidebar of case files (Game files, Display,
Graphics, Controls, Launch options) and needs no extra art files; every button, checkbox and
slider is drawn by the launcher itself.

`python pc/tools/package.py` puts the release folder together in
`build/dist/ProjectMysteryMachine` (+ .zip): both programs stripped, SDL2.dll,
libwinpthread-1.dll, a read-me and the SDL2/FFmpeg/winpthread licences; no game
data, and `recomplauncher.json` beside the programs, which
`python pc/tools/verify_release.py` also requires (it must parse and be
`format: 1`); that script rejects any disc or game-asset extensions.

`recomplauncher.json` (repository root) is RecompLauncher's manifest: it checks
the player's USA disc (`GIHE78`, plus the known Redump dump for the verified
badge), runs `scooby_launcher.exe --extract` as the windowless setup step into
`data/`, starts `scooby.exe`, and exposes the display/graphics settings in
`scooby.ini` on the game's page. Without RecompLauncher nothing changes: the
launcher and the game start exactly as before. The PC release workflow builds this archive on a tag
(`v*`) and attaches it to the GitHub release; `.github/workflows/3ds-build.yml`
is separate and only uploads a `.3dsx` artifact. The first tag therefore
publishes the launcher and game together in `ProjectMysteryMachine.zip`, with
no assets.

For a clean PC-only build:

```sh
sh pc/tools/build_ffmpeg.sh   # once
sh pc/tools/build.sh          # PC port only
python pc/tools/package.py
python pc/tools/verify_release.py
```

`sh pc/tools/rebuild.sh` remains the quicker iterative build. Neither script
touches the New 3DS build (`n3ds/tools/rebuild.sh`).

The build produces two executables from the same sources:

| Executable | Subsystem | Use |
|---|---|---|
| `build/pc/scooby.exe` | Windows GUI | release: never opens a console window |
| `build/pc/scooby_debug.exe` | console | testing: `pc_log` output and stub reports are visible (`cmake --build build/pc --target scooby_debug` if you build only that target) |

The debug exe is never copied into the release: `pc/tools/package.py` copies
only `scooby.exe` and `scooby_launcher.exe`, and `pc/tools/verify_release.py`
rejects an archive that contains it.

`scooby.ini` (pc/src/settings.cpp; `PC_SETTINGS=<file>` reads another one, for
test runs). Without it everything is as before.

| Key | Values |
|---|---|
| `game.data` | folder of the extracted disc (default: `data/` or `game/files` beside the exe) |
| `display.width`, `.height` | window size, or the exclusive fullscreen mode |
| `display.mode` | `windowed`, `borderless`, `fullscreen` |
| `display.aspect` | `auto` (the window's shape) or `W:H`; in levels the 3D view widens (Hor+), the title, menus and movies stay 4:3 |
| `display.vsync`, `.msaa` | 0/1; 0, 2, 4, 8 |
| `display.hud` | `wide`: on a wide screen each HUD element hugs its side (zHud_Render, PC_PORT); `classic`: the 4:3 layout |
| `graphics.fps` | 30 (GameCube) or 60: one VI retrace per frame; the few per-frame factors and steps scale (PC_FRAME_FACTOR/STEP in pc_compat.h), gravity on Scooby keeps 30 Hz steps so jumps match, every-frame particle calls with 1/30 emit every other frame |
| `graphics.draw_distance` | 1-8: far plane and fog (iCamera.cpp, PC_PORT) |
| `graphics.effects_distance` | 1-8: distance-culled particle emitters (zParEmitter.cpp, PC_PORT) |
| `graphics.fov` | 0-30 degrees added to the game camera (iCameraSetFOV, PC_PORT) |
| `graphics.brightness` | 0.5-2: the finished frame scaled by a blend pass (gpu_gl.cpp) |
| `graphics.shadow_res` | 256 (GameCube), 512, 1024, 2048: the shadow camera's target (SetupShadow, PC_PORT) |
| `game.skip_logos` | 1: no WB/THQ/Heavy Iron/RenderWare logo movies at start (zMenu.cpp, PC_PORT) |
| `launch.options` | extra switches for `scooby.exe`, separated by spaces: `NAME=VALUE` becomes an environment variable (`PC_DBG=nosector`, `PC_STATS=1`), any other word is added to its command line (the launcher's Launch options page) |
| `graphics.trilinear`, `.anisotropy` | mipmapped filtering for the game's bilinear textures; 1-16 |
| `keyboard.<input>` | SDL key names, comma-separated; inputs: `stick_up/down/left/right`, `cstick_*`, `a b x y start l r z`, `dpad_*` |
| `controller.<input>` | an SDL controller button or axis (`a`, `rightshoulder`, `lefttrigger`, ...) |
| `controller.sticks` | `normal` or `swapped` |
| `debug.menu` | 1: enables the debug menu (F1 or the controller's Back button); off by default |

The game has no model levels of detail (no LOD tables in the code or data);
draw and effects distance are its distance settings.

Default controls: a game controller (A/B/X/Y, triggers, sticks as on GameCube), or
keyboard: WASD/arrows move, IJKL camera, Space = A, Shift = B, E = X, R = Y,
Enter = Start, Q = L, Tab = R, F = Z, keypad 8/2/4/6 = D-pad. Alt+Enter or F11
toggles fullscreen. Saves go to `save/cardA` next to the exe.

Debug menu: off unless `debug.menu = 1` (the launcher's Controls page has the
checkbox). Once enabled, F1 (or the controller's Back/Select button) opens it. Warp to any
level and scene (from gameplay), all upgrades, unlock all warp gates, refill
health/chances/ammo, +100 Scooby Snacks, infinite health, force boss damage
(Black Knight so far), unlock movies and
credits, coordinate readout. Up/Down choose, Left/Right pick a level's scene,
Enter/A selects, Esc/B goes back; game input is ignored while it is open.
Debug keys: F3 shows the scene and Scooby's position, F4 saves it to
`coords.txt` next to the exe (and the clipboard), F5 teleports back to the last
saved spot. `PC_SCENE=<id> PC_TELEPORT=<sec>:x,y,z` starts there directly.

Debug switches (environment): `PC_SCENE=I001` starts New Game in that scene
(scene IDs and level names are in `sAreaTable`, src/Game/zScene.cpp),
`PC_PORTAL=<sec>:<n>` takes the n-th portal of the current scene,
`PC_WARP=<sec>:<scene>` warps like the menu, `PC_EVENT=<sec>:NAME:event;...` sends
events to objects, `PC_ENTINFO=<sec>:NAME` logs an entity's state, `PC_BOSSDMG=<sec>`
forces boss damage, `PC_HURT=<sec>` damages Scooby, `PC_NOMODELCULL=1` draws every model, `PC_GEOLOG=<verts>` logs
models with that vertex count, `PC_MENU=main|levels` opens the menu,
`PC_FORCESAVE=<sec>` opens the save screen, `PC_POSLOG=1` logs the position each
second, `PC_PROBE=<sec>:x0,z0,x1,z1,n` casts vertical rays (all worlds),
`PC_SPHERE=<sec>:x,y,z,r` lists world triangles near a point,
`PC_FPS=30|60` overrides the frame rate, `PC_UPGRADES=<sec>` gives every upgrade,
`PC_POSLOG=1|2|3|4` logs position (+animation, +collisions; 4: every frame), `PC_CHECKTRI=1`
checks model collision triangles against their render meshes, `PC_SURFDUMP=<sec>`
lists special surfaces, `PC_HILITE_MAT=<n>` paints a world material red, `PC_NOFMV=1` skips movies, `PC_STATS=1` logs
render stats, `PC_SHOT=frame[,step]` saves screenshots, `PC_WAV=1` records the
audio output, `PC_AUTOPAD="sec:mask,..."` scripts the pad, `PC_DBG=...`
toggles render states, `PC_FPTRAP=N` traps invalid FP operations from frame N,
and the `PC_FPCHECK` CMake option finds x87 stack leaks.

## TODO

- **Gameplay testing:** play through levels and compare with the GameCube
  build; saving in-game hasn't been exercised yet.

- **PS2 assets as an alternative source.** PS2 data is little-endian and librw
  already reads PS2-native geometry and textures. Supporting a PS2 disc needs
  the PS2 layouts where they differ from GameCube (see the decomp's PS2 DWARF notes).

## Rules for this fork

- Keep game-code changes behind `#ifdef PC_PORT` where they would change the
  GameCube build, so decomp fixes can still be merged in from `main`.
- PC-only code lives under `pc/`.
- Don't copy leaked RenderWare/Bink SDK headers or code; use librw and our own shims.
