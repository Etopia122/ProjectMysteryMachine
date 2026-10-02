#!/usr/bin/env python3

###
# Renames auto-generated symbols in config/<version>/symbols.txt to the names
# used by compiled (source) objects, so that the remaining asm and the compiled
# objects link against each other.
#
# For every unit in objdiff.json that has both a compiled object and a target
# (split) object, symbols defined at the same section offset, and relocation
# targets at the same offset, are paired. When our name is auto-generated
# (fn_/lbl_/...) and the compiled name differs, the symbol at that address is
# renamed. Run `ninja` first so both objects exist, then re-run ninja.
#
# Usage:
#   python tools/sync_symbols.py <version> [unit name substring ...]
###

import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

AUTO = re.compile(r"^(fn|lbl|gap|jumptable)_[0-9A-F]{8}$|^@")
RE_SYMBOL = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+);(.*)$")


def read_obj(path: Path):
    """Returns ({(section, offset): name} for defined symbols,
    {(section, offset): target name} for relocations)."""
    defs: Dict[Tuple[str, int], str] = {}
    relocs: Dict[Tuple[str, int], str] = {}
    with path.open("rb") as f:
        elf = ELFFile(f)
        sections = list(elf.iter_sections())
        symtab = elf.get_section_by_name(".symtab")
        syms = list(symtab.iter_symbols())
        for s in syms:
            if s["st_info"]["type"] in ("STT_FUNC", "STT_OBJECT", "STT_NOTYPE") and isinstance(s["st_shndx"], int):
                sec = sections[s["st_shndx"]].name
                if s.name and not s.name.startswith((".", "$")):
                    defs.setdefault((sec, s["st_value"]), s.name)
        for rs in sections:
            if isinstance(rs, RelocationSection):
                target_sec = sections[rs["sh_info"]].name
                for r in rs.iter_relocations():
                    sym = syms[r["r_info_sym"]]
                    name = sym.name
                    if not name and isinstance(sym["st_shndx"], int):
                        continue  # section-relative; can't pair by name
                    relocs[(target_sec, r["r_offset"])] = name
    return defs, relocs


def main() -> None:
    version = sys.argv[1]
    filters = sys.argv[2:]
    objdiff = json.loads(Path("objdiff.json").read_text())
    symbols_path = Path("config") / version / "symbols.txt"
    lines = symbols_path.read_text().splitlines()
    by_name = {}
    for i, line in enumerate(lines):
        m = RE_SYMBOL.match(line)
        if m:
            by_name[m.group(1)] = i
    existing = set(by_name)

    renames: Dict[str, str] = {}
    for unit in objdiff["units"]:
        base, target = unit.get("base_path"), unit.get("target_path")
        if not base or not target or (filters and not any(f in unit["name"] for f in filters)):
            continue
        if not Path(base).exists() or not Path(target).exists():
            continue
        bdefs, brelocs = read_obj(Path(base))
        tdefs, trelocs = read_obj(Path(target))
        pairs = [(tname, bdefs.get(key)) for key, tname in tdefs.items()]
        pairs += [(tname, brelocs.get(key)) for key, tname in trelocs.items()]
        for tname, bname in pairs:
            if bname and bname != tname and AUTO.match(tname) and not AUTO.match(bname) and not bname.startswith("..."):
                if renames.get(tname, bname) != bname:
                    print(f"conflict: {tname} -> {renames[tname]} / {bname}")
                    continue
                renames[tname] = bname

    changed = 0
    for old, new in sorted(renames.items()):
        idx = by_name.get(old)
        if idx is None:
            continue
        if new in existing:
            print(f"skip {old} -> {new}: name already used")
            continue
        m = RE_SYMBOL.match(lines[idx])
        lines[idx] = f"{new} = {m.group(2)}:0x{m.group(3)};{m.group(4)}"
        existing.add(new)
        print(f"{old} -> {new}")
        changed += 1
    symbols_path.write_text("\n".join(lines) + "\n")
    print(f"renamed {changed} symbols")


if __name__ == "__main__":
    main()
