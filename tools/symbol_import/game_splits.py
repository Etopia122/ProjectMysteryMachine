#!/usr/bin/env python3

###
# Generates .text splits for the game TUs (i*, x*, z*) from the symbol import
# reports.
#
# Anchors: functions accepted by the PS2 alignment (report.txt), plus those
# accepted by the BFBB platform alignment (report_bfbb_platform.txt) for i* TUs.
# Each TU's range starts as [first anchor, last anchor]; anchors that break the
# alphabetical link order are dropped. Ranges then grow outward over neighbour
# functions that share TU-local data (.sdata2 literals, string pools) with a
# function already inside the range. Remaining gaps are left to dtk's auto
# objects (they include GameCube-only TUs absent from the PS2 build).
#
# Usage:
#   python tools/symbol_import/game_splits.py <version> [--write]
###

import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

RE_ACCEPT = re.compile(r"^ACCEPT ([0-9A-F]{8}) gc:\s*\w+ (?:ps2|bfbb):\s*\w+ (\S+\.(?:cpp|c))\s")
RE_SPLIT_UNIT = re.compile(r"^(\S.*):")
RE_SPLIT_TEXT = re.compile(r"^\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)")
GAME_START, GAME_END = 0x800055E0, 0x800DA528
# BFBB platform TUs whose names landed on unrelated Scooby code (iFMV's
# anchors fall inside iFont, identified by its glyph table string).
BFBB_FALSE_UNITS = {"iFMV.cpp"}


def unit_path(name: str) -> str:
    if name.lower().startswith("i"):
        return f"Core/gc/{name}"
    if name.lower().startswith("x"):
        return f"Core/x/{name}"
    return f"Game/{name}"


def main() -> None:
    version = sys.argv[1]
    write = "--write" in sys.argv
    sym_dir = Path("build/symbol_import")
    gc = json.loads((sym_dir / "gc.json").read_text())
    funcs = sorted(
        (f for f in gc["functions"] if f["section"] == ".text" and GAME_START <= f["address"] < GAME_END),
        key=lambda f: f["address"],
    )
    # Drop duplicate entries (same address)
    uniq = []
    for f in funcs:
        if not uniq or uniq[-1]["address"] != f["address"]:
            uniq.append(f)
    funcs = uniq
    index = {f["address"]: i for i, f in enumerate(funcs)}
    strings = gc["strings"]

    def local_data(f) -> Set[str]:
        return {
            s
            for s, sec in f["refs"].items()
            if sec == ".sdata2" or (sec in (".rodata", ".data") and s in strings)
        }

    data = [local_data(f) for f in funcs]

    anchors: Dict[str, List[int]] = {}
    for report, platform in ((sym_dir / "report.txt", False), (sym_dir / "report_bfbb_platform.txt", True)):
        for line in report.read_text().splitlines():
            m = RE_ACCEPT.match(line)
            if not m:
                continue
            unit = m.group(2)
            # PS2 anchors for all TUs (rewritten platform TUs are already
            # rejected there); BFBB anchors only for the platform layer
            if platform and not unit.lower().startswith("i"):
                continue
            if platform and unit in BFBB_FALSE_UNITS:
                continue
            addr = int(m.group(1), 16)
            if addr in index:
                anchors.setdefault(unit, []).append(index[addr])

    # Existing splits (already-defined TUs) take precedence
    splits_path = Path("config") / version / "splits.txt"
    existing: Dict[str, Tuple[int, int]] = {}
    cur = None
    for line in splits_path.read_text().splitlines():
        m = RE_SPLIT_UNIT.match(line)
        if m and not line.startswith(("\t", " ")) and not line.startswith("Sections"):
            cur = m.group(1)
            continue
        m = RE_SPLIT_TEXT.match(line)
        if m and cur:
            existing[cur] = (int(m.group(1), 16), int(m.group(2), 16))

    units = sorted(anchors, key=str.lower)
    # Enforce monotonic order: keep each unit's anchors that lie between the
    # median anchors of its neighbours
    medians = {u: sorted(anchors[u])[len(anchors[u]) // 2] for u in units}
    ranges: Dict[str, List[int]] = {}
    for k, u in enumerate(units):
        lo = medians[units[k - 1]] if k > 0 else -1
        hi = medians[units[k + 1]] if k + 1 < len(units) else len(funcs)
        kept = sorted(i for i in anchors[u] if lo < i < hi)
        if kept:
            ranges[u] = [kept[0], kept[-1]]
    units = [u for u in units if u in ranges]
    # Resolve overlaps between consecutive units by trimming to the boundary
    for a, b in zip(units, units[1:]):
        if ranges[a][1] >= ranges[b][0]:
            print(f"# overlap {a} / {b}; trimming", file=sys.stderr)
            ranges[a][1] = ranges[b][0] - 1

    # Grow ranges over neighbours sharing TU-local data
    taken = [None] * len(funcs)
    for u in units:
        for i in range(ranges[u][0], ranges[u][1] + 1):
            taken[i] = u
    for path, (s, e) in existing.items():
        for i, f in enumerate(funcs):
            if s <= f["address"] < e:
                taken[i] = path
    changed = True
    while changed:
        changed = False
        for u in units:
            lo, hi = ranges[u]
            inside = set().union(*(data[i] for i in range(lo, hi + 1)))
            if lo > 0 and taken[lo - 1] is None and data[lo - 1] & inside:
                ranges[u][0] -= 1
                taken[lo - 1] = u
                changed = True
            if hi + 1 < len(funcs) and taken[hi + 1] is None and data[hi + 1] & inside:
                ranges[u][1] += 1
                taken[hi + 1] = u
                changed = True

    out = []
    for u in units:
        path = unit_path(u)
        if path in existing:
            continue
        lo, hi = ranges[u]
        if any(taken[i] not in (u, None) for i in range(lo, hi + 1)):
            print(f"# {u}: range collides with an existing split; skipped", file=sys.stderr)
            continue
        start = funcs[lo]["address"]
        end = funcs[hi]["address"] + funcs[hi]["size"]
        out.append((start, f"\n{path}:\n\t.text       start:0x{start:08X} end:0x{end:08X}"))
        print(f"{start:08X}-{end:08X} {hi - lo + 1:4} fns  {path}", file=sys.stderr)
    covered = sum(1 for t in taken if t)
    print(f"{len(out)} new TUs; {covered}/{len(funcs)} game functions covered", file=sys.stderr)
    if write:
        with splits_path.open("a") as f:
            for _, text in sorted(out):
                f.write(text + "\n")


if __name__ == "__main__":
    main()
