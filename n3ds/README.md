# Scooby-Doo! Night of 100 Frights — New 3DS port (experimental)

A port of the PC port (`pc/`) to the New Nintendo 3DS / New 2DS XL only: it
relies on the 804 MHz CPU mode, the larger memory budget, the C-stick and ZL/ZR.
The game data comes from the player's own GameCube disc and lives on the SD card.

This is a separate build from the PC release. Use `sh n3ds/tools/rebuild.sh` for
the 3DS app; the PC workflow and `pc/tools/package.py` never include 3DS files,
and the 3DS workflow only uploads `scooby.3dsx` as an artifact. A CIA that
contains game data is for your own console only and must not be published.

## Status (2026-10-10, Azahar emulator only)

- Boots through the copyright screen and title to the main menu; New Game
  loads the Mystic Manor hub. Every one of the 87 levels loads and holds the
  game's 30 fps cap, standing at the start and walking about (`perf.sh`;
  2026-10-10, Azahar: the emulator doesn't model GPU time, so real hardware
  is untested). What it took: collision trees for world and model queries,
  GPU skinning (`skin.v.pica`), skinning each pose once, keyed vertex reuse
  and only setting citro3d state that changed. Music plays (ndsp); fog works.
- Debug menu (Select): level warps, upgrades, Force Boss Damage etc. as on PC;
  ZL+X saves the position, ZL+Y returns to it. The PC_* test switches work
  from `env.txt` (PC_SCENE, PC_WARP, PC_TELEPORT, PC_POSLOG, ...).
- Saves: the game finds and reads existing saves (copied from the PC port);
  writing is the same shared file code but hasn't been exercised on the 3DS
  (the save/load card screens don't react to scripted input on either port).
- Movies: the Bink files are re-encoded once on a PC (`sh n3ds/tools/convert_fmv.sh
  game/files <out>`: Ogg Theora 320x240 + 32 kHz Vorbis, 59 MB for all 26) and
  copied to `sdmc:/scooby/fmv`. Theora decodes in software (~3-6 ms a frame);
  the hardware Y2R unit converts each picture straight into the GPU texture
  (0.6 ms; `PC_FMV_CPU=1` uses the CPU instead). Full 29.97 fps with sound.
- HUD on the bottom screen: `zHud_Render` draws there (its 640x480 space at
  half size), and the HUD stays up during play instead of sliding away after
  two seconds; it still hides for cutscenes. `PC_HUD_TOP=1` puts it back on
  the top screen with the original timing (the bottom screen then shows the
  text console).
- 3DS touches: saving and loading go straight to the save slots (no memory
  card screen) and the texts talk about save data, not GameCube memory cards
  (`pc/src/text_patch.cpp`, both ports); button prompts show 3DS buttons
  (`n3ds/src/button_art.cpp`) and the texts call the GameCube's Z ZR; warp
  gates don't open the warp screen where they stand any more: walking by one
  makes a WARP button appear on the bottom screen, and touching it brings up
  the warp screen for every gate opened so far. Loading a save brings the button
  back: the save has the gates it opened, and port_flags.txt beside the saves
  remembers a walk by the hub's gate (open from the start, so the save can't).
- Widescreen in play: in a level the 3D view fills the whole top screen
  (400x240, 5:3) with a wider field of view (frustum too, so nothing is culled
  at the new edges); 2D made for 4:3 stays 4:3 in the middle, untextured
  full-screen 2D (fades, letterbox) spans the screen and full-screen pictures
  (the warp map) get black side bars. The intro, title and front-end menus stay
  4:3; movies too. `PC_ASPECT43=1` keeps play 4:3. Every heavy level still
  holds 30 fps. The title screen and front-end menus are widescreen too: the
  title's 4:3 backdrop gives way to the player's 5:3 picture
  (`game/n3ds_title_bg.png` -> `title_bg.rgba`, through `pc_texture_replace`),
  drawn over the whole screen at the backdrop's depth (behind the gang).
- Bottom screen background: a 320x240 picture behind the HUD (and on the
  title and menus), from `romfs:/bottom_bg.rgba` or `sdmc:/scooby/bottom_bg.rgba`,
  else a dark purple gradient. `make_cia.sh` converts the player's
  `game/n3ds_bottom_bg.png` (the title screen's forest without its logo; game art,
  so not in the repository) with `n3ds/tools/make_bottom_bg.py`.
- Not done yet: lines/points (no such primitives), real-hardware testing.

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
- Real hardware: copy `scooby.3dsx` to `sdmc:/3ds/`, the disc's `files`
  folder to `sdmc:/scooby/files` and the converted movies to `sdmc:/scooby/fmv`; start it from the Homebrew Launcher.
  Controls: Circle Pad = stick, C-stick = camera, A/B/X/Y, L/R, ZR = Z,
  Start; Start+Select quits.

## CIA

`sh n3ds/tools/make_cia.sh` packages `build/n3ds/scooby.cia` (and `scooby.cxi`,
which emulators boot directly) with makerom and bannertool (in
`C:/Users/Ethan/Tools/3ds`, or `$CTR_TOOLS`). By default the game files (without
the Bink movies, which the 3DS doesn't use) and the converted movies go inside
the RomFS: one ~672 MB file to install with FBI, nothing else to copy (all 38
movies, the title screen's attract-mode demos included). That CIA contains the
game's data: it is for your own console only and must not be shared.
`--no-fmv` makes `scooby_nofmv.cia`/`.cxi` without the movies, for testing:
they're skipped unless `sdmc:/scooby/fmv` has them (same title ID, so installing
one replaces the other). `--no-data` makes an app-only CIA that reads
`sdmc:/scooby` like the .3dsx. A `PC_NOFMV=1` line in `sdmc:/scooby/env.txt`
skips every movie, in any build. The app looks in `romfs:/files` and `romfs:/fmv` first; saves, the log
and `env.txt` stay in `sdmc:/scooby`. The header (`n3ds/cia.rsf`) asks for the
New 3DS 124 MB memory mode, 804 MHz with L2 cache and core 2. The HOME Menu
icon and banner are made from the disc's GameCube memory card icon/banner
(`make_cia_art.py`); the banner's picture and sound are the player's own
`game/n3ds_banner.png` (256x128) and `game/n3ds_banner.wav` (up to ~3 s)
instead when they are there.

## Tools

- `sh n3ds/tools/run.sh <app.3dsx> <seconds> [shot times...]`: runs in Azahar,
  saves window screenshots to `build/n3ds/shot_<t>.png`.
- `n3ds/tools/cropscale.ps1`: crops/enlarges a screenshot.
- gdb: set `use_gdbstub=true` (and `use_gdbstub\default=false`) in Azahar's
  `qt-config.ini`; `arm-none-eabi-gdb` with `set tcp auto-retry on` attaches
  before the game gets far, and `handle SIGSEGV nostop noprint nopass` skips
  the stub's spurious stops. Breakpoints and hardware watchpoints work;
  interrupting a running target in batch mode doesn't, and other threads'
  registers are stale.
- `sh n3ds/tools/sample.sh <warmup> <count>`: sampling profiler. The app's
  sample thread (`N3DS_SAMPLE`, added for the run) hits a breakpoint four times
  a second and gdb records the main thread's stack; prints the innermost and
  the inclusive function counts (`build/n3ds/samples.txt` has the stacks).
- `sh n3ds/tools/perf.sh [-o table] SCENE...|all`: frame-rate table per scene
  (average and lowest fps, frame time, skinning/lighting/drawing, wait for the
  retrace, draws, vertices copied), standing at the start or with
  `PERF_WALK=1` walking about; `-q QUEUE` shares one scene list between
  emulator copies. Run a copy of the script for long sweeps (sh reads a script
  as it goes).
- A second Azahar copy (renamed executable, its own `user` folder) can run
  beside the first: `AZAHAR=<dir> AZAHAR_EXE=<exe>` for run.sh, sample.sh and
  perf.sh. Two at once disturb the emulated clock (one-second dips that don't
  happen alone), so measure lowest frame rates with one running.
- `N3DS_O0_UNITS=<regex>` (CMake) builds matching game sources at -O0.
- `sh n3ds/tools/scenes.sh F001 G001 ...`: New Game into each scene,
  screenshot, frame rate and null-pointer reads (Azahar logs unmapped reads
  as "UnmappedAccess" instead of crashing). `N3DS_ENV="PC_DBG=..."` adds switches.
- `sh n3ds/tools/shots.sh <app> <time>...` with `N3DS_ENV="..."`: runs with test
  switches and screenshots at moments of game time. Test runs save to
  `sdmc:/scooby/testsave` (`PC_SAVE_DIR`), never the player's saves; perf.sh,
  scenes.sh and sample.sh set it too. `PC_ALLWARPS=<sec>` opens every warp gate,
  `PC_AUTOTOUCH=<sec>:<x>:<y>` taps the bottom screen, `PC_WARPSEEN=<sec>` counts as
  walking by a gate, `PC_FORCESAVE=<sec>` opens the save screen,
  `PC_SNACKS=<sec>:<n>` collects n snacks and `PC_TOKENS=<sec>:<n>` gives the
  first n monster tokens, both through the game's pickup path; `PC_CLUES=<sec>:<n>`
  gives n clues as a pickup does (player inventory and level count);
  `PC_DEATHS=<sec>:<n>` counts n deaths as losing a life does;
  `PC_SCARES=<sec>:<n>` counts n scares the same way; `PC_AREAS=<sec>:<n>` marks
  the first n real areas' snacks all found; `PC_CUTSCENES=<sec>:<n>` marks n
  cutscenes seen; `PC_MASTERMIND=<sec>` beats the Mastermind through the game's
  own call (which also sets the endgame flag).
- `PC_DBG=cpucolour|cpustatic|cpuuniform|nocache` (shared renderer) narrow
  down vertex colour problems to one path.

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
  screen framebuffer is rotated (see `load_mvp`). Perspective draws use a
  W-buffer so the fog table is linear in eye distance.
- Decompiled functions that fall off the end without a return (-Wreturn-type)
  break at -O2: GCC treats the end as unreachable (VILN_*_respond jumped to 0).
- Geometry the game locks (RpGeometryLock) is rewritten at run time: the GPU
  backend must not cache it.
- File-scope initialisers run before the 3DS main sets the environment from
  env.txt: read PC_* switches on first use.
