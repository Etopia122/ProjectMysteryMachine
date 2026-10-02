#!/usr/bin/env python3

###
# Compiles one unit and prints per-function match percentages from objdiff.
# Also reports whether the compiled function order matches the DOL order.
#
# Usage:
#   python tools/check_unit.py <version> <unit substring, e.g. Core/x/xString>
###

import json
import subprocess
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

OBJDIFF = "build/tools/objdiff-cli.exe" if sys.platform == "win32" else "build/tools/objdiff-cli"


def func_order(path: Path):
    with path.open("rb") as f:
        elf = ELFFile(f)
        syms = [
            (s["st_value"], s.name)
            for s in elf.get_section_by_name(".symtab").iter_symbols()
            if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] != "SHN_UNDEF"
        ]
    return [n for _, n in sorted(syms)]


def main() -> None:
    version, needle = sys.argv[1], sys.argv[2]
    units = json.loads(Path("objdiff.json").read_text())["units"]
    unit = next((u for u in units if needle in u["name"]), None)
    if unit is None:
        sys.exit(f"no unit matching {needle}")
    base, target = unit.get("base_path"), unit["target_path"]
    ninja = ["ninja", base] if base else ["ninja"]
    proc = subprocess.run(ninja, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit(proc.stdout[-4000:] + proc.stderr[-2000:])
    report = Path("build") / version / "unit_report.json"
    proc = subprocess.run(
        [str(Path(OBJDIFF).resolve()), "report", "generate", "-o", str(report), "-f", "json"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        sys.exit(proc.stderr)
    data = json.loads(report.read_text())
    for u in data["units"]:
        if u["name"] != unit["name"]:
            continue
        total = 0.0
        fns = u.get("functions", [])
        for f in fns:
            pct = f.get("fuzzy_match_percent", 0.0)
            total += pct
            print(f"{pct:7.2f}%  {f['name']}")
        for s in u.get("sections", []):
            if s.get("fuzzy_match_percent", 100.0) < 100.0:
                print(f"section {s['name']}: {s.get('fuzzy_match_percent', 0.0):.2f}%")
        if fns:
            print(f"average {total / len(fns):.2f}% over {len(fns)} functions")
    if base and Path(base).exists():
        ours, theirs = func_order(Path(base)), func_order(Path(target))
        common = [n for n in theirs if n in ours]
        order_ok = [n for n in ours if n in common] == common
        print("function order:", "matches DOL" if order_ok else f"DIFFERS (DOL: {common})")


if __name__ == "__main__":
    main()
