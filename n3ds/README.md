# Scooby-Doo! Night of 100 Frights — New 3DS port (experimental)

A port of the PC port (`pc/`) to the New Nintendo 3DS / New 2DS XL only: it
relies on the 804 MHz CPU mode, the larger memory budget, the C-stick and ZL/ZR.
The game data comes from the player's own GameCube disc and lives on the SD card.

## Status

- Toolchain, emulator and test probe working (`n3ds/probe`): citro3d renders,
  New 3DS mode detected, 96 MB app memory (91 MB largest malloc, 30 MB linear),
  game files read from `sdmc:/scooby/files` at ~5 MB/s in Azahar.
- Next: make the PC platform layer portable (renderer backend split, no Win32 in
  shared code), then the 3DS platform layer and a citro3d renderer.

## Setup

- devkitPro 3DS toolchain in MSYS2 (`C:/msys64/opt/devkitpro`): devkitPro's
  pacman repositories (`[dkp-libs]`, `[dkp-windows]`) and the `3ds-dev` group.
- Builds use the mingw32 cmake + ninja the PC build uses, with
  `n3ds/cmake/n3ds.cmake` as the toolchain file (devkitPro's own CMake files
  require MSYS2's cmake):

```
export PATH=/c/msys64/mingw32/bin:$PATH
cmake -S n3ds/probe -B build/n3ds-probe -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/n3ds/cmake/n3ds.cmake
cmake --build build/n3ds-probe
```

- Emulator: Azahar (portable: a `user` folder next to `azahar.exe`), New 3DS
  mode on, log filter `*:Info Debug.Emulated:Debug`. Its SD card is
  `user/sdmc`; `user/sdmc/scooby/files` is a junction to `game/files`.
- `sh n3ds/tools/run.sh <app.3dsx> <seconds> [shot times...]` runs an app,
  saves window screenshots to `build/n3ds/shot_<t>.png` and prints the app's
  debug output (only the first `svcOutputDebugString` line reaches Azahar's log,
  so the bottom-screen console is the main readout).
- Real hardware: copy the `.3dsx` to `sdmc:/3ds/` and the disc's `files` folder
  to `sdmc:/scooby/files`, then start it from the Homebrew Launcher.
