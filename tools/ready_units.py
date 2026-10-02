#!/usr/bin/env python3

###
# Lists split units without source, smallest first, with the number of calls
# to unnamed (fn_*) functions in each -- units with none are easy to start.
#
# Usage:
#   python tools/ready_units.py <version> [max text size, hex]
###

import re
import sys
from pathlib import Path


def main() -> None:
    version = sys.argv[1]
    limit = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x800
    asm_root = Path("build") / version / "asm"
    cur = None
    units = []
    for line in (Path("config") / version / "splits.txt").read_text().splitlines():
        if line and not line[0].isspace() and line.endswith(":"):
            cur = line[:-1]
        m = re.match(r"\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)", line)
        if m and cur and cur.startswith(("Game/", "Core/")):
            size = int(m.group(2), 16) - int(m.group(1), 16)
            if not Path("src", cur).exists() and size <= limit:
                units.append((size, cur))
    for size, unit in sorted(units):
        asm = asm_root / (unit.rsplit(".", 1)[0] + ".s")
        text = asm.read_text() if asm.exists() else ""
        unknown = sorted(set(re.findall(r"\bbl (fn_[0-9A-F]{8})", text)))
        funcs = len(re.findall(r"^\.fn ", text, re.M))
        print(f"{size:6X} {funcs:3} fns  {len(unknown):2} unnamed callees  {unit}  {' '.join(unknown[:4])}")


if __name__ == "__main__":
    main()
