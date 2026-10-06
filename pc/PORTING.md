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

1. **Toolchain.** Build 32-bit first (asset structs and RenderWare data assume
   4-byte pointers). Needs an i686 compiler, CMake and SDL2
   (MSYS2: `mingw-w64-i686-gcc`, `mingw-w64-i686-cmake`, `mingw-w64-i686-SDL2`).
   64-bit comes later, once pointer fields in asset layouts are handled.
2. **Name the leftovers.** Resolve the `fn_*`/`lbl_*` placeholders to the real
   SDK/RW/game functions and data they stand for (the decomp's symbols.txt and asm).
3. **Link with stubs.** Generate stub definitions for every import so the whole
   game links into an executable that does nothing useful yet.
4. **Assets and endianness.** HIP archives and their contents are big-endian on
   GameCube; textures and geometry are GameCube-native RenderWare data. Either
   byte-swap/convert at load time, or use another release's assets (PS2 data is
   little-endian and librw reads PS2-native geometry and textures). Decide before step 5.
5. **Platform layer.** Replace `src/Core/gc/i*.cpp` piece by piece:
   files (DVD -> stdio), memory (OS heap -> malloc), time, pad (SDL gamepad),
   rendering (RenderWare via librw on OpenGL), sound (AX -> SDL audio, DSP-ADPCM
   decoder), saves (CARD -> files), FMV (Bink -> FFmpeg or skip).
6. **Boot to the title screen**, then into a level, then fix gameplay bugs by
   comparing with the GameCube build (Dolphin + the decomp's debugging tools).

## Rules for this fork

- Keep game-code changes behind `#ifdef PC_PORT` where they would change the
  GameCube build, so decomp fixes can still be merged in from `main`.
- PC-only code lives under `pc/`.
- Don't copy leaked RenderWare/Bink SDK headers or code; use librw and our own shims.
