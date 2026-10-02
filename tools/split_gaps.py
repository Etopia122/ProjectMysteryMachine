#!/usr/bin/env python3
"""List unassigned .text ranges next to configured units.

A gap right before or after a unit often holds that unit's first or last
functions (the bulk-imported splits miss them). Usage:
    python tools/split_gaps.py GIHE78 [--max 0x400]
"""
import re
import sys
from pathlib import Path


def main():
    version = sys.argv[1]
    limit = 0x400
    if "--max" in sys.argv:
        limit = int(sys.argv[sys.argv.index("--max") + 1], 0)
    cfg = Path("config") / version
    units = []
    cur = None
    for line in (cfg / "splits.txt").read_text().splitlines():
        if line and not line.startswith(("\t", " ")) and line.endswith(":"):
            cur = line[:-1]
        m = re.match(r"\s+\.text\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)", line)
        if m and cur:
            units.append((int(m.group(1), 16), int(m.group(2), 16), cur))
    units.sort()
    syms = []
    for line in (cfg / "symbols.txt").read_text().splitlines():
        m = re.match(r"(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)", line)
        if m:
            syms.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
    syms.sort()
    status = {}
    for line in Path("configure.py").read_text().splitlines():
        m = re.search(r'Object\((\w+), "([^"]+)"', line)
        if m:
            status[m.group(2)] = m.group(1)
    for (s0, e0, u0), (s1, e1, u1) in zip(units, units[1:]):
        if s1 > e0 and s1 - e0 <= limit:
            inside = [n for a, sz, n in syms if e0 <= a < s1]
            print(f"{e0:08X}-{s1:08X} ({s1 - e0:#x}) after {u0} [{status.get(u0, '?')}] "
                  f"before {u1} [{status.get(u1, '?')}]: {' '.join(inside)}")


if __name__ == "__main__":
    main()
