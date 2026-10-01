#!/usr/bin/env python3

###
# Helpers for reading BFBB's (GQPE78) symbols.txt / splits.txt, used as a
# reference for GameCube mangled names and sizes of shared engine functions.
###

import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple

RE_SYMBOL = re.compile(
    r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // type:(\w+)(?: size:0x([0-9A-Fa-f]+))?"
)
RE_SPLIT_UNIT = re.compile(r"^(\S.*):$")
RE_SPLIT_SECTION = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)")


def demangle_key(mangled: str) -> Optional[str]:
    """Reduces a CodeWarrior mangled name to 'Class::name' or 'name'."""
    if mangled.startswith("@") or mangled.startswith("fn_") or mangled.startswith("lbl_"):
        return None
    # Find the "__" separating the base name from the qualifier/signature,
    # skipping leading underscores (e.g. __ct, __dt, __as operators).
    i = 2 if mangled.startswith("__") else 0
    sep = mangled.find("__", i)
    if sep <= 0:
        return mangled
    base, rest = mangled[:sep], mangled[sep + 2 :]
    m = re.match(r"^(\d+)", rest)
    if m:
        n = int(m.group(1))
        cls = rest[len(m.group(1)) : len(m.group(1)) + n]
        base = {"__ct": cls, "__dt": "~" + cls}.get(base, base)
        return f"{cls}::{base}"
    if rest.startswith("Q"):
        # Nested qualifiers: Q<count><len><name>...
        parts = []
        j = 2
        for _ in range(int(rest[1])):
            m = re.match(r"^(\d+)", rest[j:])
            if not m:
                break
            n = int(m.group(1))
            j += len(m.group(1))
            parts.append(rest[j : j + n])
            j += n
        return "::".join(parts + [base])
    return base


def load_symbols(path: Path) -> List[Tuple[str, str, int, str, int]]:
    out = []
    for line in path.read_text().splitlines():
        m = RE_SYMBOL.match(line)
        if m:
            out.append(
                (m.group(1), m.group(2), int(m.group(3), 16), m.group(4), int(m.group(5), 16) if m.group(5) else 0)
            )
    return out


def load_text_units(path: Path) -> List[Tuple[str, int, int]]:
    """Returns (unit, start, end) for each .text split, in link order."""
    out = []
    unit = None
    for line in path.read_text().splitlines():
        m = RE_SPLIT_UNIT.match(line)
        if m and not line.startswith("\t"):
            unit = m.group(1)
            continue
        m = RE_SPLIT_SECTION.match(line)
        if m and unit and m.group(1) == ".text":
            out.append((unit, int(m.group(2), 16), int(m.group(3), 16)))
    return out


def functions_by_unit(
    symbols_path: Path, splits_path: Path, full_path: bool = False
) -> Dict[str, List[Dict]]:
    syms = [s for s in load_symbols(symbols_path) if s[1] == ".text" and s[3] == "function"]
    syms.sort(key=lambda s: s[2])
    units = load_text_units(splits_path)
    result: Dict[str, List[Dict]] = {}
    ui = 0
    for name, _, addr, _, size in syms:
        while ui < len(units) and addr >= units[ui][2]:
            ui += 1
        if ui < len(units) and units[ui][1] <= addr < units[ui][2]:
            unit = units[ui][0] if full_path else units[ui][0].split("/")[-1]
            result.setdefault(unit, []).append(
                {"mangled": name, "key": demangle_key(name), "address": addr, "size": size}
            )
    return result
