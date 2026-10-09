# Scooby-Doo! Night of 100 Frights — New 3DS port (experimental)

A port of the PC port (`pc/`) to the New Nintendo 3DS / New 2DS XL only: it
relies on the 804 MHz CPU mode, the larger memory budget, the C-stick and ZL/ZR.
The game data comes from the player's own GameCube disc and lives on the SD card.

## Status (2026-10-09, Azahar emulator only)

- Boots through the copyright screen and title to the main menu; New Game
  loads the Mystic Manor hub with Scooby controllable. Music plays (ndsp).
- Speed: title/menu ~20 fps, hub ~10 fps (the game targets 30). The CPU
  still lights and skins models and copies dynamic vertices every frame.
- Not done yet: fog (the PICA200 fogs by depth through a lookup table),
  lines/points (no such primitives), FMV (skipped), the debug menu (PC-only),
  bottom-screen use, saves untested, no real-hardware testing yet.

## Layout

- `n3ds/src`: entry point and platform hooks (`main.cpp`), pad (`pad.cpp`),
  ndsp audio output, the citro3d graphics backend (`gpu_c3d.cpp`, shader
  `scene.v.pica`), generated stubs.
- Shared with the PC port: the game code, `pc/rw` (RenderWare on top of the
  `gpu.h` backend interface; `gpu_gl.cpp` is the PC backend) and most of
  `pc/src` (files, ARAM, AX audio mixing, CARD saves, VI/OS time, threads via
  `pc_thread_start`).

## Setup

- devkitPro 3DS toolchain in MSYS2 (`C:/msys64/opt/devkitpro`): devkitPro's
  pacman repositories (`[dkp-libs]`, `[dkp-windows]`) and the `3ds-dev` group.
- Build: `sh n3ds/tools/rebuild.sh` (mingw32 cmake + ninja with
  `n3ds/cmake/n3ds.cmake`; devkitPro's own CMake files require MSYS2's cmake).
  Output: `build/n3ds/scooby.3dsx` (ELF with symbols: `build/n3ds/scooby`).
- Emulator: Azahar (portable: a `user` folder next to `azahar.exe`), New 3DS
  mode on. Its SD card is `user/sdmc`; `user/sdmc/scooby/files` is a junction
  to `game/files`. ndsp needs `sdmc:/3ds/dspfirm.cdc` (a dump from the
  player's own console; Azahar accepts a placeholder file).
- On the SD card: `scooby/files` (disc files), `scooby/log.txt` (the log),
  `scooby/env.txt` (the PC port's `PC_*` debug switches as KEY=VALUE lines,
  e.g. `PC_NOFMV=1`, `PC_PROF=1`, `PC_AUTOPAD=...`, `PC_LOG_FILES=1`), saves in
  `scooby/save`.
- Real hardware: copy `scooby.3dsx` to `sdmc:/3ds/` and the disc's `files`
  folder to `sdmc:/scooby/files`; start it from the Homebrew Launcher.
  Controls: Circle Pad = stick, C-stick = camera, A/B/X/Y, L/R, ZR = Z,
  Start; Start+Select quits.

## Tools

- `sh n3ds/tools/run.sh <app.3dsx> <seconds> [shot times...]`: runs in Azahar,
  saves window screenshots to `build/n3ds/shot_<t>.png`.
- `n3ds/tools/cropscale.ps1`: crops/enlarges a screenshot.
- gdb: set `use_gdbstub=true` (and `use_gdbstub\default=false`) in Azahar's
  `qt-config.ini`; `arm-none-eabi-gdb` with `set tcp auto-retry on` attaches
  before the game gets far, and `handle SIGSEGV nostop noprint nopass` skips
  the stub's spurious stops. Breakpoints and hardware watchpoints work;
  interrupting a running target in batch mode doesn't, and other threads'
  registers are stale (`sample.sh` is not reliable).
- `N3DS_O0_UNITS=<regex>` (CMake) builds matching game sources at -O0.

## Pitfalls found

- Helper threads (VI retrace, interrupts, audio) must run above the game's
  priority or on core 2: 3DS threads aren't time-sliced and the game
  busy-waits on them.
- The game read the GameCube bus clock from 0x800000F8 in several files: an
  unmapped read returns 0 in Azahar (infinite timestep, NaN player, collision
  against every polygon). All reads use the constant under `PC_PORT`.
- devkitARM sizes enums by their values; game and shared code need
  `-fno-short-enums`, while code using libctru/citro3d structs must not.
- Inline functions some game sources call out of line (xBaseEnable & co.)
  only got emitted at -O0: optimised builds need real definitions.
- `stat` per file is slow on the SD card; the disc scan uses `d_type`.
- GPU-visible memory written by the CPU must be flushed
  (`GSPGPU_FlushDataCache`); Azahar doesn't need it but hardware does.
- PICA200 textures: tiled 8x8 Morton, rows stored bottom to top; the top
  screen framebuffer is rotated (see `load_mvp`).
