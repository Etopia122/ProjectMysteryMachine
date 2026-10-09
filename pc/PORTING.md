# Scooby-Doo! Night of 100 Frights — PC port

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
   (`fmv.cpp`: Bink via FFmpeg, replacing `iFMV.cpp`). Boots through the logo
   movies and Intro to the title screen and main menu; New Game loads the
   first level, where Scooby can run around; memory card saves go to files
   (`card.cpp`).
6. **Boot to the title screen**, then into a level, then fix gameplay bugs by
   comparing with the GameCube build (Dolphin + the decomp's debugging tools).

## Running

```
sh pc/tools/rebuild.sh          # build
python pc/tools/deploy.py       # copy the MinGW/SDL2/FFmpeg DLLs next to the exe
build/pc/scooby.exe             # data from SCOOBY_DATA or game/files
```

Controls: a game controller (A/B/X/Y, triggers, sticks as on GameCube), or
keyboard: WASD/arrows move, IJKL camera, Space = A, Shift = B, E = X, R = Y,
Enter = Start, Q = L, Tab = R, F = Z, keypad 8/2/4/6 = D-pad. Alt+Enter or F11
toggles fullscreen. Saves go to `save/cardA` next to the exe.

Debug menu: F1 (or the controller's Back/Select button) opens it. Warp to any
level and scene (from gameplay), all upgrades, unlock all warp gates, refill
health/chances/ammo, +100 Scooby Snacks, infinite health, unlock movies and
credits, coordinate readout. Up/Down choose, Left/Right pick a level's scene,
Enter/A selects, Esc/B goes back; game input is ignored while it is open.
Debug keys: F3 shows the scene and Scooby's position, F4 saves it to
`coords.txt` next to the exe (and the clipboard), F5 teleports back to the last
saved spot. `PC_SCENE=<id> PC_TELEPORT=<sec>:x,y,z` starts there directly.

Debug switches (environment): `PC_SCENE=I001` starts New Game in that scene
(scene IDs and level names are in `sAreaTable`, src/Game/zScene.cpp),
`PC_PORTAL=<sec>:<n>` takes the n-th portal of the current scene,
`PC_WARP=<sec>:<scene>` warps like the menu, `PC_MENU=main|levels` opens the menu,
`PC_FORCESAVE=<sec>` opens the save screen, `PC_POSLOG=1` logs the position each
second, `PC_PROBE=<sec>:x0,z0,x1,z1,n` casts vertical rays, `PC_SURFDUMP=<sec>`
lists special surfaces, `PC_HILITE_MAT=<n>` paints a world material red, `PC_NOFMV=1` skips movies, `PC_STATS=1` logs
render stats, `PC_SHOT=frame[,step]` saves screenshots, `PC_WAV=1` records the
audio output, `PC_AUTOPAD="sec:mask,..."` scripts the pad, `PC_DBG=...`
toggles render states, `PC_FPTRAP=N` traps invalid FP operations from frame N,
and the `PC_FPCHECK` CMake option finds x87 stack leaks.

## TODO

- **Gameplay testing:** play through levels and compare with the GameCube
  build; saving in-game hasn't been exercised yet.
- **Remaining GX direct calls** (screen effects that use GX copies).
- **Trim FFmpeg:** the MSYS2 build pulls in ~70 DLLs; a Bink-only build would do.

- **PS2 assets as an alternative source.** PS2 data is little-endian and librw
  already reads PS2-native geometry and textures. Supporting a PS2 disc needs
  the PS2 layouts where they differ from GameCube (see the decomp's PS2 DWARF notes).

## Rules for this fork

- Keep game-code changes behind `#ifdef PC_PORT` where they would change the
  GameCube build, so decomp fixes can still be merged in from `main`.
- PC-only code lives under `pc/`.
- Don't copy leaked RenderWare/Bink SDK headers or code; use librw and our own shims.
