#!/usr/bin/env python3

###
# Shrinks symbols in config/<version>/symbols.txt that a split boundary cuts
# through ("Split ... ends within symbol"). dtk's guessed sizes often include
# alignment padding that really belongs between units.
#
# Only symbols that start inside a split range and run past its end are
# shrunk (to end at the split's end). Prints every change.
#
# Usage:
#   python tools/fit_symbols.py <version>
###

import re
import sys
from pathlib import Path

RE_SPLIT = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)")
RE_SYM = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // (.*?)size:0x([0-9A-Fa-f]+)(.*)$")


def pad_split_ends(version: str) -> None:
    """Extends unaligned split ends to the next symbol when only padding (< 8
    bytes) lies in between; dtk requires the following split to be aligned."""
    starts = {}
    for line in (Path("config") / version / "symbols.txt").read_text().splitlines():
        m = RE_SYM.match(line)
        if m:
            starts.setdefault(m.group(2), []).append(int(m.group(3), 16))
    for v in starts.values():
        v.sort()
    path = Path("config") / version / "splits.txt"
    lines = path.read_text().splitlines()
    for i, line in enumerate(lines):
        m = RE_SPLIT.match(line)
        if not m:
            continue
        sec, s, e = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
        if e % 4 == 0:
            continue
        nxt = next((a for a in starts.get(sec, []) if a >= e), None)
        new = nxt if nxt is not None and nxt - e < 8 else (e + 3) & ~3
        lines[i] = line.replace(f"end:0x{m.group(3)}", f"end:0x{new:08X}")
        print(f"split {sec} {s:08X}: end {e:08X} -> {new:08X}")
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    version = sys.argv[1]
    pad_split_ends(version)
    ranges = {}
    for line in (Path("config") / version / "splits.txt").read_text().splitlines():
        m = RE_SPLIT.match(line)
        if m:
            ranges.setdefault(m.group(1), []).append((int(m.group(2), 16), int(m.group(3), 16)))
    path = Path("config") / version / "symbols.txt"
    lines = path.read_text().splitlines()
    changed = 0
    for i, line in enumerate(lines):
        m = RE_SYM.match(line)
        if not m:
            continue
        sec, addr, size = m.group(2), int(m.group(3), 16), int(m.group(5), 16)
        for s, e in ranges.get(sec, []):
            if s <= addr < e < addr + size:
                lines[i] = f"{m.group(1)} = {sec}:0x{addr:08X}; // {m.group(4)}size:0x{e - addr:X}{m.group(6)}"
                print(f"{m.group(1)}: size 0x{size:X} -> 0x{e - addr:X}")
                changed += 1
                break
    path.write_text("\n".join(lines) + "\n")
    print(f"{changed} symbols shrunk")


if __name__ == "__main__":
    main()
