# Libraries and memory layout (GIHE78)

## Layout of `.text`

| Range | Contents |
| --- | --- |
| 0x800055E0 - ~0x80016CA8 | Platform code (`Core/gc/i*`), alphabetical |
| ~0x80016CA8 - 0x80044780 | `Core/x/x*`, alphabetical (case-insensitive) |
| 0x80044780 - 0x800D7E00 | `Game/z*`, alphabetical |
| 0x800D7E00 - 0x800DE530 | Second group of platform units linked after the game: `iAnim`, `iSnd`, `iFMV`, `iFX`, `iMix` (an older Dolphin MIX library) |
| 0x800DE530 - 0x800E5774 | MetroTRK |
| 0x800E5774 - 0x800F391C | Runtime + MSL |
| 0x800F391C - 0x80111EC0 | Dolphin SDK |
| 0x80111EC0 - 0x80130AF4 | Bink |
| 0x80130AF4 - 0x80137214 | Dolphin SDK (CARD) |
| 0x80137214 - 0x8018E5C0 | RenderWare |

The second platform group's unit names are chosen by role (BFBB names where
the code is recognisably the same module); its real file names are unknown.
Scooby's GameCube build has no `zVolume` code.

## Where library sources come from

| Library | Source | Notes |
| --- | --- | --- |
| MetroTRK, Runtime, MSL | [zeldaret/tww](https://github.com/zeldaret/tww) (CC0) | Byte-identical to The Wind Waker's copies. TWW headers live in `include/tww`. Two Scooby differences: `math.h`'s `sqrtf` must not emit per-unit constants (`-DSCOOBY_MSL`), and `e_pow.c` must not take `one`'s address. |
| Dolphin SDK (`dolphin/src/...`) | [bfbbdecomp/bfbb](https://github.com/bfbbdecomp/bfbb) (CC0) | `DolphinLib` in `configure.py`. |
| Dolphin SDK (`dolphin/<lib>/...`) | zeldaret/tww (CC0) | `tww_*` libraries with `cflags_tww_dolphin`. |

Not used: Melee's decomp (no license), RenderWare headers from BFBB's repo
(proprietary SDK headers; `iAnim.cpp` re-expresses the one slerp macro it
needs).

## Workflow for a reference library

1. `python tools/place_ref_objects.py GIHE78 <ref root> <lib> ... --range <lo>-<hi> --splits out.txt --renames names.txt`
   compiles the reference's sources with its own flags (or `--flags`/`--mw`
   overrides) and finds each object in our DOL. `MATCH` = the whole object
   matches; `CHAIN` = the surviving functions match in order (the rest were
   dead-stripped, or differ). Data sections are located through relocations
   and symbol names.
2. Add the non-overlapping units as NonMatching with their splits
   (`tools/check_split_order.py` and `tools/fit_symbols.py` fix ordering and
   symbol-size problems), apply `names.txt` with `tools/rename.py` (skip
   alias labels such as `_savefpr_14`).
3. `python tools/test_units.py --keep <units...>` switches each unit to
   Matching and keeps the ones for which the DOL still matches.

Caveats:

- The masked comparison ignores relocation targets, so wrappers that differ
  only in what they call look identical (`math_ppc.c`'s float wrappers matched
  the wrong functions). The hash check catches this.
- A CHAIN often covers only part of a unit; the other functions are a
  different (older) version. Such units link-fail with "multiply-defined" when
  marked Matching and stay NonMatching.

## Known issues

- A split boundary at 0x8010774C (`OSSync.c`: `SystemCallVector` with the
  zero-size labels `__OSSystemCallVectorStart/End`) makes `mwldeppc` hang,
  whether the unit is assembly or compiled. `OSRtc.c` and `OSSync.c` are
  therefore not split yet.
