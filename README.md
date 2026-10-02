Scooby-Doo! Night of 100 Frights
=================================

A work-in-progress decompilation of *Scooby-Doo! Night of 100 Frights* for the
Nintendo GameCube (Heavy Iron Studios / THQ, 2002).

This repository does **not** contain any game assets or assembly. An existing
copy of the game is required.

Supported versions:

- `GIHE78`: Rev 0 (USA) — `main.dol` SHA-1 `c77fb29d39b5898933e8ef4d1ccd2ddac404554e`

Status
======

- The project builds a `main.dol` that is byte-identical to the retail one.
- Symbols: ~70% of `.text` is named, from the PS2 build's DWARF data
  ([dwarfdb](https://github.com/seilweiss/dwarfdb)) and reference GameCube
  decomps (BFBB, Melee, Wind Waker). See `tools/symbol_import/README.md`.
- Compiler: CodeWarrior for GameCube 1.3.2, `-inline deferred`, signed chars
  (see `docs/compiler.md`).
- Matching source: `Core/x/xBase.cpp` and several Dolphin SDK files (SDK
  sources from [bfbbdecomp/bfbb](https://github.com/bfbbdecomp/bfbb), CC0).

Dependencies
============

Windows
--------

Use native tooling; WSL or msys2 are not required.

- Install [Python](https://www.python.org/downloads/) and add it to `%PATH%`.
- Install [ninja](https://github.com/ninja-build/ninja/releases) and add it to `%PATH%`.
- `pip install pyelftools capstone` (used by the helper tools in `tools/`).

macOS / Linux
-------------

- Install ninja. [wibo](https://github.com/decompals/wibo) is downloaded
  automatically to run the Windows compilers.

Building
========

- Copy your game's disc image to `orig/GIHE78`.
  - **ISO/GCM only**: `tools/prepare_dol.py` reads `main.dol` directly from the
    image. It extracts `orig/GIHE78/sys/main.dol`; keep that file if you delete
    the disc image.

- Configure:

  ```sh
  python configure.py
  ```

- Build:

  ```sh
  ninja
  ```

  The build checks the result against `config/GIHE78/build.sha1`.

Notes
-----

- The retail DOL's entry point is a stack-fill routine (0x80016898) that calls
  `__start`. decomp-toolkit locates `.init` from the entry point, so analysis
  runs on `orig/GIHE78/main_analysis.dol`, a copy with the entry point set to
  `__start`; the link uses `-m fn_80016898` to restore the original.
- The linker script (`config/GIHE78/ldscript.tpl`) uses a 0x1000 debugger stack.

Diffing
=======

After a successful build, `objdiff.json` exists in the project root. Open the
project directory in [objdiff](https://github.com/encounter/objdiff) and pick an
object from the sidebar. Changes to sources, headers, `configure.py`,
`splits.txt` or `symbols.txt` rebuild automatically.

Useful tools:

- `python tools/asm_fn.py GIHE78 <name or address>` — print a function's asm
- `python tools/rename.py GIHE78 0x80001234=Name` — rename a symbol
- `python tools/sync_symbols.py GIHE78` — adopt names from compiled objects
- `python tools/import_splits.py GIHE78 <ref symbols> <ref splits> <TU>` —
  derive splits for TUs shared with another decomp
- `python tools/compiler_probe/probe.py GIHE78 <src>` — test compilers/flags
- `python tools/check_unit.py GIHE78 <unit>` — per-function match % and order check
- `sh tools/diff_fn.sh <src> <function>` — side-by-side diff with the real flags
- `python tools/tu_info.py GIHE78 <unit>` — PS2 signatures, data ranges and asm for a unit
- `python tools/ready_units.py GIHE78` — undecompiled units by size, with unnamed callees
- `python tools/m2c_fn.py GIHE78 <function>` — first C draft of a function via
  [m2c](https://github.com/matt-kempster/m2c)
  (`pip install git+https://github.com/matt-kempster/m2c.git`)
