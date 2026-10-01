#!/usr/bin/env python3

###
# Reports how many functions (and code bytes) are named, per library region.
#
# Usage:
#   python tools/symbol_import/coverage.py <version>
###

import json
import re
import sys
from pathlib import Path

RE_FUNC = re.compile(r"^(\S+) = \.(?:text|init):0x([0-9A-F]+); // type:function size:0x([0-9A-F]+)")


def main() -> None:
    version = sys.argv[1]
    regions = [
        (int(r["start"], 16), int(r["end"], 16), r["family"])
        for r in json.loads((Path("config") / version / "library_regions.json").read_text())["regions"]
    ]
    regions.insert(0, (0x80003100, regions[0][0], "game code (i/x/z) + .init"))
    stats = {r[2]: [0, 0, 0, 0] for r in regions}
    for line in (Path("config") / version / "symbols.txt").read_text().splitlines():
        m = RE_FUNC.match(line)
        if not m:
            continue
        addr, size = int(m.group(2), 16), int(m.group(3), 16)
        named = not m.group(1).startswith("fn_")
        for lo, hi, fam in regions:
            if lo <= addr < hi:
                s = stats[fam]
                s[0] += 1
                s[1] += named
                s[2] += size
                s[3] += size * named
    total = [0, 0, 0, 0]
    for fam, s in stats.items():
        print(f"{fam:56} {s[1]:5}/{s[0]:<5} functions {100 * s[3] // max(s[2], 1):3}% of bytes")
        total = [a + b for a, b in zip(total, s)]
    print(f"{'TOTAL':56} {total[1]:5}/{total[0]:<5} functions {100 * total[3] // total[2]:3}% of bytes")


if __name__ == "__main__":
    main()
