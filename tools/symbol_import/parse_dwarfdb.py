#!/usr/bin/env python3

###
# Parses a dwarfdb dump (https://github.com/seilweiss/dwarfdb) into JSON.
#
# Output: a list of compile units sorted by code address (i.e. link order),
# each with its functions (name, signature, PS2 address range) and globals.
#
# Usage:
#   python tools/symbol_import/parse_dwarfdb.py ref/dwarfdb/scooby/SLUS-20349/1.0 out.json
###

import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, List

RE_UNIT = re.compile(r"^\s+Compile unit: (.+)$")
RE_CODE_RANGE = re.compile(r"^\s+Code range: 0x([0-9A-F]+) -> 0x([0-9A-F]+)$")
RE_RANGE = re.compile(r"^// Range: 0x([0-9A-F]+) -> 0x([0-9A-F]+)$")
RE_GLOBAL = re.compile(r"^(?!//)(.+); // size: 0x([0-9A-F]+), address: 0x([0-9A-F]+)$")
RE_FUNC_NAME = re.compile(r"([~\w:<>,\s\*&]+?)\s*\(")


def strip_param_comments(sig: str) -> str:
    return re.sub(r"\s*/\*.*?\*/", "", sig)


def func_name(sig: str) -> str:
    # Find the name immediately before the parameter list (handles operators)
    head = sig.split("(", 1)[0]
    if "operator" in head:
        return head[head.rindex(" ", 0, head.index("operator")) + 1 :].strip()
    return head.split()[-1].lstrip("*&")


def parse_file(path: Path, units: Dict[str, Dict[str, Any]]) -> None:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    unit_name = None
    i = 0
    while i < len(lines):
        line = lines[i]
        m = RE_UNIT.match(line)
        if m:
            unit_name = m.group(1).strip()
            unit = units.setdefault(
                unit_name, {"name": unit_name, "functions": [], "globals": []}
            )
            i += 1
            continue
        m = RE_RANGE.match(line)
        if m and unit_name and i + 1 < len(lines):
            start, end = int(m.group(1), 16), int(m.group(2), 16)
            # Methods have a "// this: <reg>" line before the signature
            while i + 1 < len(lines) and lines[i + 1].startswith("// "):
                i += 1
            sig = lines[i + 1].rstrip()
            if sig.endswith("{"):
                sig = strip_param_comments(sig[:-1].strip())
                units[unit_name]["functions"].append(
                    {
                        "name": func_name(sig),
                        "signature": sig,
                        "static": sig.startswith("static "),
                        "start": start,
                        "end": end,
                    }
                )
                i += 2
                continue
        m = RE_GLOBAL.match(line)
        if m and unit_name:
            decl, size, addr = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
            units[unit_name]["globals"].append(
                {
                    "decl": decl,
                    "is_function": decl.rstrip().endswith(")"),
                    "static": decl.startswith("static "),
                    "size": size,
                    "address": addr,
                }
            )
        i += 1


def main() -> None:
    root, out = Path(sys.argv[1]), Path(sys.argv[2])
    units: Dict[str, Dict[str, Any]] = {}
    for path in sorted(root.rglob("*")):
        if path.is_file():
            parse_file(path, units)

    result: List[Dict[str, Any]] = []
    for unit in units.values():
        # Deduplicate functions emitted in multiple dumps (e.g. inlines in headers)
        seen = set()
        funcs = []
        for f in sorted(unit["functions"], key=lambda f: f["start"]):
            key = (f["start"], f["name"])
            if key not in seen:
                seen.add(key)
                funcs.append(f)
        unit["functions"] = funcs
        unit["start"] = funcs[0]["start"] if funcs else None
        unit["end"] = funcs[-1]["end"] if funcs else None
        result.append(unit)
    result.sort(key=lambda u: (u["start"] is None, u["start"] or 0))
    out.write_text(json.dumps(result, indent=1))

    for u in result:
        if u["start"] is not None:
            print(
                f"{u['start']:#08x}-{u['end']:#08x} {len(u['functions']):4} fns  {u['name']}"
            )


if __name__ == "__main__":
    main()
