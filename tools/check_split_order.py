#!/usr/bin/env python3

###
# Reports units whose sections are ordered inconsistently with their .text
# (dtk then fails with "Cyclic dependency encountered while resolving link
# order"). For every section, units sorted by .text start must also be sorted
# by that section's start.
#
# Usage:
#   python tools/check_split_order.py <version>
###

import re
import sys
from pathlib import Path

RE_U = re.compile(r"^(\S.*):\s*$")
RE_S = re.compile(r"^\s+(\.\w+)\s+start:0x(\w+) end:0x(\w+)")


def main() -> None:
    units = {}
    cur = None
    for line in (Path("config") / sys.argv[1] / "splits.txt").read_text().splitlines():
        m = RE_U.match(line)
        if m and line != "Sections:":
            cur = m.group(1)
            units[cur] = {}
            continue
        m = RE_S.match(line)
        if m and cur:
            units[cur][m.group(1)] = int(m.group(2), 16)
    order = sorted((u for u in units if ".text" in units[u]), key=lambda u: units[u][".text"])
    secs = {s for u in units.values() for s in u if s != ".text"}
    bad = 0
    for sec in sorted(secs):
        seq = [(u, units[u][sec]) for u in order if sec in units[u]]
        for (u1, a1), (u2, a2) in zip(seq, seq[1:]):
            if a2 < a1:
                print(f"{sec}: {u2} ({a2:08X}) comes before {u1} ({a1:08X}) but its .text is later")
                bad += 1
    print(f"{bad} inversions")


if __name__ == "__main__":
    main()
