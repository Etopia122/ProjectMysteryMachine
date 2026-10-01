#!/usr/bin/env python3

###
# Aligns all PS2 game functions against the GameCube .text in one pass.
#
# The GameCube build links game TUs in case-insensitive alphabetical order
# (i*, x*, z*), and CodeWarrior emits functions in source order, so the PS2
# functions concatenated in that TU order form a sequence that aligns with the
# GC function sequence. Alignment is global (Needleman-Wunsch) on function
# sizes, with TU-local data references (string pools, float literals) used to
# penalize assigning functions that share data to different TUs.
#
# Usage:
#   python tools/symbol_import/global_align.py ps2.json gc.json <start> <end> out.json
###

import json
import math
import sys
from pathlib import Path
from typing import Any, Dict, List

# PS2-only TUs (platform code with no GameCube counterpart)
PS2_ONLY = (
    "ps2_mpeg/",
    "p2rwcoll/",
    "p2skyfs.c",
    "p2im3dhack.cpp",
    "ms4sound.cpp",
    "fakeprintf.c",
    "gcc_wrapper.c",
)

GAP_PS2 = -1.0
GAP_GC = -0.8
TU_BREAK_SHARED = -6.0  # TU change between two GC functions that share local data


def pair_score(p: int, g: int) -> float:
    if p <= 8 or g <= 8:
        return 0.6 if abs(p - g) <= 8 else -1.5
    d = abs(math.log(g / p))
    return max(-2.5, 2.5 - 7.0 * d)


def tu_sort_key(name: str) -> str:
    return name.lower()


def main() -> None:
    ps2 = json.loads(Path(sys.argv[1]).read_text())
    gc = json.loads(Path(sys.argv[2]).read_text())
    start, end = int(sys.argv[3], 16), int(sys.argv[4], 16)
    out = Path(sys.argv[5])

    units = []
    for u in ps2:
        path = u["name"].replace("\\", "/")
        base = path.split("/")[-1]
        if base.endswith(".h") or not u["functions"] or any(k in path for k in PS2_ONLY):
            continue
        units.append((base, u))
    units.sort(key=lambda x: tu_sort_key(x[0]))

    seq: List[Dict[str, Any]] = []
    for base, u in units:
        for f in u["functions"]:
            seq.append(
                {
                    "unit": base,
                    "unit_path": u["name"],
                    "size": f["end"] - f["start"],
                    "name": f["name"],
                    "signature": f["signature"],
                    "static": f["static"],
                }
            )

    funcs = [f for f in gc["functions"] if f["section"] == ".text" and start <= f["address"] < end]
    strings = gc["strings"]

    # shared[j]: GC function j shares TU-local data with function j-1
    local_sets = []
    for f in funcs:
        s = set()
        for sym, sec in f["refs"].items():
            if sec == ".sdata2" or (sec in (".rodata", ".data") and sym in strings):
                s.add(sym)
        local_sets.append(s)
    # Propagate: functions between two users of the same object are in one TU
    users: Dict[str, List[int]] = {}
    for j, s in enumerate(local_sets):
        for sym in s:
            users.setdefault(sym, []).append(j)
    same_as_prev = [False] * len(funcs)
    for js in users.values():
        for j in range(min(js) + 1, max(js) + 1):
            same_as_prev[j] = True

    n, m = len(seq), len(funcs)
    print(f"aligning {n} PS2 functions ({len(units)} TUs) against {m} GC functions", file=sys.stderr)

    NEG = -1e18
    # Standard NW with traceback; store scores row by row
    prev = [GAP_GC * j for j in range(m + 1)]
    trace = [bytearray([3] * (m + 1))]
    trace[0][0] = 0
    for i in range(1, n + 1):
        p = seq[i - 1]
        cur = [NEG] * (m + 1)
        tr = bytearray(m + 1)
        cur[0] = prev[0] + GAP_PS2
        tr[0] = 2
        unit_changes = i == 1 or seq[i - 2]["unit"] != p["unit"]
        for j in range(1, m + 1):
            diag = prev[j - 1] + pair_score(p["size"], funcs[j - 1]["size"])
            if unit_changes and same_as_prev[j - 1]:
                diag += TU_BREAK_SHARED
            up = prev[j] + GAP_PS2
            left = cur[j - 1] + GAP_GC
            if diag >= up and diag >= left:
                cur[j], tr[j] = diag, 1
            elif up >= left:
                cur[j], tr[j] = up, 2
            else:
                cur[j], tr[j] = left, 3
        trace.append(tr)
        prev = cur
        if i % 200 == 0:
            print(f"  row {i}/{n}", file=sys.stderr)

    pairs = []
    i, j = n, m
    while i > 0 or j > 0:
        t = trace[i][j]
        if t == 1:
            pairs.append((i - 1, j - 1))
            i, j = i - 1, j - 1
        elif t == 2:
            i -= 1
        else:
            j -= 1
    pairs.reverse()

    matched = []
    for i, j in pairs:
        p, f = seq[i], funcs[j]
        matched.append(
            {
                "gc": f["address"],
                "gc_index": j,
                "ps2_index": i,
                "gc_size": f["size"],
                "ps2_size": p["size"],
                "unit": p["unit"],
                "unit_path": p["unit_path"],
                "name": p["name"],
                "signature": p["signature"],
                "static": p["static"],
                "score": round(pair_score(p["size"], f["size"]), 2),
            }
        )
    out.write_text(json.dumps(matched, indent=1))

    # Summarize per TU
    by_unit: Dict[str, List[Dict[str, Any]]] = {}
    for mt in matched:
        by_unit.setdefault(mt["unit"], []).append(mt)
    for base, u in units:
        ms = by_unit.get(base, [])
        if not ms:
            print(f"{'':17} {base:24}   0/{len(u['functions'])}")
            continue
        good = sum(1 for x in ms if x["score"] >= 1.5)
        print(f"{ms[0]['gc']:08X}-{ms[-1]['gc'] + ms[-1]['gc_size']:08X} {base:24} {len(ms):3}/{len(u['functions']):3}  good {good}")


if __name__ == "__main__":
    main()
