#!/usr/bin/env python3

###
# Suggests data section ranges for a TU from the objects its functions
# reference, bounded by what neighbouring TUs reference.
#
# For each data section, prints the objects referenced by the TU's .text and
# the nearest objects referenced by the previous/next TUs (by link order), so
# the TU's range can be chosen: it must contain its own references and must not
# overlap a neighbour's. TU-local sections (.sdata2 literals, string pools) give
# tight bounds; .data/.bss/.sbss globals may be referenced from other TUs, so
# check those by hand.
#
# Usage:
#   python tools/data_splits.py <version> <unit, e.g. Core/x/xMath.cpp>
###

import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

RE_UNIT = re.compile(r"^(\S.*):")
RE_SEC = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)")
DATA_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")


def main() -> None:
    version, unit = sys.argv[1], sys.argv[2]
    splits: Dict[str, Dict[str, Tuple[int, int]]] = {}
    cur = None
    for line in (Path("config") / version / "splits.txt").read_text().splitlines():
        m = RE_UNIT.match(line)
        if m and not line.startswith(("\t", " ")) and not line.startswith("Sections"):
            cur = m.group(1)
            splits[cur] = {}
            continue
        m = RE_SEC.match(line)
        if m and cur:
            splits[cur][m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    if unit not in splits or ".text" not in splits[unit]:
        sys.exit(f"{unit} has no .text split")

    gc = json.loads(Path("build/symbol_import/gc.json").read_text())
    objs = gc["objects"]
    by_addr = {o["address"]: (n, o) for n, o in objs.items()}

    def refs_of(text_range) -> Dict[str, List[int]]:
        out: Dict[str, List[int]] = {}
        for f in gc["functions"]:
            if text_range[0] <= f["address"] < text_range[1]:
                for sym, sec in f["refs"].items():
                    if sym in objs and sec in DATA_SECTIONS:
                        out.setdefault(sec, []).append(objs[sym]["address"])
        return {k: sorted(set(v)) for k, v in out.items()}

    order = sorted((v[".text"][0], u) for u, v in splits.items() if ".text" in v)
    idx = [u for _, u in order].index(unit)
    prev_u = order[idx - 1][1] if idx > 0 else None
    next_u = order[idx + 1][1] if idx + 1 < len(order) else None
    mine = refs_of(splits[unit][".text"])
    prev = refs_of(splits[prev_u][".text"]) if prev_u else {}
    nxt = refs_of(splits[next_u][".text"]) if next_u else {}

    print(f"{unit}: .text {splits[unit]['.text'][0]:08X}-{splits[unit]['.text'][1]:08X}")
    print(f"  prev TU: {prev_u}\n  next TU: {next_u}")
    for sec in DATA_SECTIONS:
        if sec not in mine:
            continue
        lo, hi = mine[sec][0], mine[sec][-1]
        hi_end = hi + by_addr[hi][1]["size"]
        p = [a for a in prev.get(sec, []) if a < lo]
        n = [a for a in nxt.get(sec, []) if a > hi]
        print(f"  {sec}: refs {', '.join(f'{a:08X}' for a in mine[sec])}")
        print(f"      own span {lo:08X}-{hi_end:08X}"
              f"; prev TU's last ref {max(p):08X}" if p else f"      own span {lo:08X}-{hi_end:08X}; prev TU: none",
              end="")
        print(f"; next TU's first ref {min(n):08X}" if n else "; next TU: none")
        # list all objects between the neighbour bounds
        lo_b = max(p) if p else lo
        hi_b = min(n) if n else hi_end
        for a in sorted(by_addr):
            if lo_b <= a <= hi_b and objs[by_addr[a][0]]["section"] == sec:
                name, o = by_addr[a]
                mark = "*" if a in mine[sec] else " "
                print(f"        {mark} {a:08X} size {o['size']:4X} {name}")


if __name__ == "__main__":
    main()
