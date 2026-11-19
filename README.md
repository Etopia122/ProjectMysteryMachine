PROJECT: Mystery Machine
=================================

A work-in-progress decompilation of *Scooby-Doo! Night of 100 Frights* for the
Nintendo GameCube (Heavy Iron Studios / THQ, 2002).

This repository does **not** contain any game assets or assembly. An existing
copy of the game is required.

Supported versions:

- `GIHE78`: Rev 0 (USA) — `main.dol` SHA-1 `c77fb29d39b5898933e8ef4d1ccd2ddac404554e`

Fan project and legal notice
============================

This is an unofficial, non-commercial fan project. It is not affiliated with,
sponsored by or endorsed by Warner Bros. Discovery, Hanna-Barbera, Cartoon
Network, THQ, Heavy Iron Studios or Nintendo. *Scooby-Doo! Night of 100
Frights*, Scooby-Doo and all related characters, names, music and logos are the
property of their respective owners.

Neither this repository nor the PC release archive contains game data: no disc
image, no extracted files, no models, textures, audio or video. You need your
own copy of the game (USA, `GIHE78`) to build or play. Dumping your own disc is
your responsibility under your local law; this project does not host or
redistribute any part of the game. `tools/check_no_game_assets.py` enforces
this: it runs on every push and pull request and fails on a tracked disc image,
extracted asset, archive or oversized blob.

The decompiled sources in `src/`, `include/` and `config/` are derived from the
retail game for study and for building a matching binary. They remain the
copyright of the game's owners, and nothing here grants a licence to them, to
the game, or to any character.

The PC launcher also ships two pieces of third-party clip art — a Scooby-Doo
portrait (also used for the icon) and the Mystery Machine van — that their
sources offer for **personal use only**. They are included so the launcher
looks as intended and are embedded in the built `scooby_launcher.exe`; they are
**not** licensed to anyone for redistribution, reuse or commercial use, and
are removed on request. [`NOTICE`](NOTICE) has the details and the takedown
contact; [Licence](#licence) says exactly what the CC0 licence covers.

Ports
=====

The decompilation and the two ports are separate builds:

- `pc/` builds **Project: Mystery Machine**, the native Windows port
  (`scooby.exe` and `scooby_launcher.exe`). `sh pc/tools/build.sh` builds only
  this port; `python pc/tools/package.py` makes the asset-free
  `ProjectMysteryMachine.zip` release archive.
- `n3ds/` builds the experimental New 3DS port (`.3dsx`/CIA). It has its own
  `n3ds/tools/rebuild.sh` and is never part of a PC release.

The PC launcher extracts the game files from the player's own USA GameCube ISO
(`GIHE78`) and starts the game. No game assets are stored in this repository or
in the PC release archive. Releases are published from the archive built by
`python pc/tools/package.py`; `.github/workflows/pc-release.yml` can be run by
hand to check the build, and the 3DS job is separate and only uploads a
`.3dsx` artifact.

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

Licence
=======

What is licensed, and what is not:

- **Project code and tooling**: the port and build tooling in `pc/`, `n3ds/`
  and `tools/`, plus the decompilation template itself, are released under
  **CC0 1.0**, see [`LICENSE`](LICENSE).
- **Decompiled game code** (`src/`, `include/`, `config/`): not ours to license.
  It is derived from the retail game, remains the copyright of its owners, and
  is included only so the matching binary can be built and studied. It is not
  covered by the CC0 licence above.
- **Third-party software**: SDL2 (zlib licence), libwinpthread (mingw-w64, MIT)
  and FFmpeg (LGPL 2.1 or later; only its Bink decoders, linked into
  `scooby.exe`). The PC release archive ships their licence texts in
  `licences/`, together with the FFmpeg source it was built from.
- **Launcher artwork**: see the table below. It is not covered by the CC0
  licence.

The full statement is in [`NOTICE`](NOTICE), which the release archive also
carries as `licences/FAN-PROJECT-NOTICE.txt`.

Launcher artwork
----------------

| File | What it is | Status |
|---|---|---|
| `pc/launcher/logo.png` | "PROJECT: MYSTERY MACHINE" wordmark | Project artwork, supplied by the author |
| `pc/launcher/scooby.png` | Scooby-Doo portrait: the header badge | Third-party clip art, **personal use only — not licensed for redistribution** |
| `pc/launcher/van.png` | Mystery Machine van in the header | Third-party clip art, **personal use only — not licensed for redistribution** |
| `pc/launcher/launcher.ico` | Window, taskbar and file icon (16–256 px) | Rendered from `scooby.png`, so the same restriction applies |

The two clip-art files are placeholders and can be replaced with original
artwork at any time. The launcher draws its own Great Dane badge and van in
`pc/launcher/launcher.cpp` when those bitmap resources are absent, so deleting
`scooby.png`, `van.png` and `launcher.ico` is enough to ship without third-party
art at all.

If you are a rights holder and want something removed, open an issue and it will
be taken down.
