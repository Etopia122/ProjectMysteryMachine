# Symbol import

Scripts that name GameCube (GIHE78) functions using the PS2 retail build's
DWARF data and BFBB's GameCube symbols. They were run once to seed
`config/GIHE78/symbols.txt`; re-run them only against a symbols.txt that still
has the functions you want renamed as `fn_XXXXXXXX` (names are never
overwritten).

## Inputs (not committed, see `/ref` in .gitignore)

```
git clone --depth 1 --filter=blob:none --sparse https://github.com/seilweiss/dwarfdb.git ref/dwarfdb
git -C ref/dwarfdb sparse-checkout set scooby/SLUS-20349
curl -o ref/bfbb/symbols.txt https://raw.githubusercontent.com/bfbbdecomp/bfbb/main/config/GQPE78/symbols.txt
curl -o ref/bfbb/splits.txt  https://raw.githubusercontent.com/bfbbdecomp/bfbb/main/config/GQPE78/splits.txt
```

## Pipeline

```
python tools/symbol_import/parse_dwarfdb.py ref/dwarfdb/scooby/SLUS-20349/1.0 build/symbol_import/ps2.json
python tools/symbol_import/analyze_gc.py GIHE78 build/symbol_import/gc.json
python tools/symbol_import/global_align.py build/symbol_import/ps2.json build/symbol_import/gc.json 800055E0 800DE530 build/symbol_import/aligned.json
python tools/symbol_import/apply_names.py GIHE78 build/symbol_import/ps2.json build/symbol_import/aligned.json build/symbol_import/report.txt
python tools/symbol_import/align_bfbb_platform.py GIHE78 build/symbol_import/gc.json 800055E0 80016CA8 build/symbol_import/report_bfbb_platform.txt
```

`analyze_gc.py` needs the asm from a `ninja` run. `segment_gc.py` and
`match_tus.py` are exploratory tools (data-reference segmentation and
per-TU local alignment) kept for investigating TU boundaries.

## How it works

- The GameCube game code is linked in case-insensitive alphabetical TU order
  (`i*`, then `x*`, then `z*`), from 0x800055E0 to the MetroTRK code at
  0x800DE530.
- Scooby's compiler emits a TU's functions in the same order as the PS2 build
  (reverse source order; BFBB's newer setup emits source order).
- PS2 (MIPS) and GC (PPC) function sizes are close to 1:1, so the PS2
  functions, concatenated in alphabetical TU order, are aligned to the GC
  functions by size (Needleman-Wunsch). Shared float literals / string pools
  penalize TU breaks between functions that must be in the same TU.
- Matches are accepted when sizes agree within 0.8-1.25x, or when they fill a
  one-to-one gap between accepted matches in the same TU. Mangled names are
  generated from the DWARF signatures (`mangle.py`), using full prototypes when
  a definition dropped unused parameters.
- PS2-specific platform files (iSnd, iSystem, iFile, ...) are skipped; the
  `i*` region is named from BFBB's GameCube `i*` TUs instead (strict sizes).

## Caveats

- DWARF from the PS2 build has no `const`, so some mangled names may lack `C`
  qualifiers that the GameCube build has. BFBB-derived names use BFBB's type
  names (`xVec3`), PS2-derived ones use Scooby's tags (`_xVec3`).
- `build/symbol_import/report.txt` lists every accepted and rejected match.
