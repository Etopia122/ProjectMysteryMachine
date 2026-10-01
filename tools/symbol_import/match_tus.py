#!/usr/bin/env python3

###
# Locates PS2 translation units in the GameCube .text by aligning function
# size sequences (Smith-Waterman local alignment), then resolves overlaps
# greedily by score.
#
# MIPS (PS2) and PPC (GC) function sizes for this codebase are close to 1:1,
# and CodeWarrior emits functions in source order on both platforms.
#
# Usage:
#   python tools/symbol_import/match_tus.py ps2.json gc.json segments.json out.json
###

import json
import math
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

GAP_PS2 = -1.2  # PS2 function missing on GC
GAP_GC = -1.0  # extra GC function (e.g. new code, different inlining)
MIN_SCORE = 6.0


def pair_score(p: int, g: int) -> float:
    if p <= 8 or g <= 8:
        # Tiny stubs carry little information
        return 0.5 if abs(p - g) <= 8 else -1.0
    d = abs(math.log(g / p))
    return max(-2.0, 2.0 - 6.0 * d)


def align(ps2_sizes: List[int], gc_sizes: List[int]) -> Tuple[float, List[Tuple[int, int]]]:
    n, m = len(ps2_sizes), len(gc_sizes)
    # H[i][j]: best local alignment ending at ps2 i, gc j
    prev = [0.0] * (m + 1)
    trace: List[bytearray] = [bytearray(m + 1)]
    best, best_i, best_j = 0.0, 0, 0
    for i in range(1, n + 1):
        cur = [0.0] * (m + 1)
        tr = bytearray(m + 1)
        p = ps2_sizes[i - 1]
        for j in range(1, m + 1):
            diag = prev[j - 1] + pair_score(p, gc_sizes[j - 1])
            up = prev[j] + GAP_PS2
            left = cur[j - 1] + GAP_GC
            v, t = 0.0, 0
            if diag > v:
                v, t = diag, 1
            if up > v:
                v, t = up, 2
            if left > v:
                v, t = left, 3
            cur[j] = v
            tr[j] = t
            if v > best:
                best, best_i, best_j = v, i, j
        trace.append(tr)
        prev = cur
    pairs = []
    i, j = best_i, best_j
    while i > 0 and j > 0:
        t = trace[i][j]
        if t == 0:
            break
        if t == 1:
            pairs.append((i - 1, j - 1))
            i, j = i - 1, j - 1
        elif t == 2:
            i -= 1
        else:
            j -= 1
    pairs.reverse()
    return best, pairs


def main() -> None:
    ps2 = json.loads(Path(sys.argv[1]).read_text())
    gc = json.loads(Path(sys.argv[2]).read_text())
    out = Path(sys.argv[4])
    funcs = [f for f in gc["functions"] if f["section"] == ".text"]
    # Game code ends where the runtime/SDK libraries begin (TRK/MSL)
    game_end = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0x800DE000
    game = [f for f in funcs if f["address"] < game_end]
    gc_sizes = [f["size"] for f in game]

    candidates: List[Dict[str, Any]] = []
    for unit in ps2:
        name = unit["name"].replace("\\", "/").split("/")[-1]
        if name.endswith(".h") or len(unit["functions"]) < 2:
            continue
        sizes = [f["end"] - f["start"] for f in unit["functions"]]
        score, pairs = align(sizes, gc_sizes)
        if score < MIN_SCORE or not pairs:
            continue
        candidates.append(
            {
                "unit": name,
                "score": score,
                "norm": score / len(sizes),
                "pairs": pairs,
                "gc_first": pairs[0][1],
                "gc_last": pairs[-1][1],
                "ps2_count": len(sizes),
            }
        )
        print(f"  {name:24} score {score:7.1f} ({len(pairs)}/{len(sizes)} fns) @ {game[pairs[0][1]]['address']:08X}", file=sys.stderr)

    # Greedy placement by score, rejecting overlaps
    candidates.sort(key=lambda c: -c["score"])
    taken: List[Tuple[int, int]] = []
    placed = []
    for c in candidates:
        lo, hi = c["gc_first"], c["gc_last"]
        if any(not (hi < a or lo > b) for a, b in taken):
            continue
        taken.append((lo, hi))
        placed.append(c)
    placed.sort(key=lambda c: c["gc_first"])

    result = []
    units = {u["name"].replace("\\", "/").split("/")[-1]: u for u in ps2}
    for c in placed:
        u = units[c["unit"]]
        result.append(
            {
                "unit": c["unit"],
                "score": round(c["score"], 1),
                "start": game[c["gc_first"]]["address"],
                "end": game[c["gc_last"]]["address"] + game[c["gc_last"]]["size"],
                "matched": len(c["pairs"]),
                "ps2_count": c["ps2_count"],
                "functions": [
                    {
                        "gc": game[j]["address"],
                        "gc_size": game[j]["size"],
                        "ps2_size": u["functions"][i]["end"] - u["functions"][i]["start"],
                        "name": u["functions"][i]["name"],
                        "signature": u["functions"][i]["signature"],
                    }
                    for i, j in c["pairs"]
                ],
            }
        )
        print(
            f"{result[-1]['start']:08X}-{result[-1]['end']:08X} {c['unit']:24} score {c['score']:7.1f}  {len(c['pairs'])}/{c['ps2_count']}"
        )
    out.write_text(json.dumps(result, indent=1))
    print(f"placed {len(placed)} of {len(candidates)} candidate units")


if __name__ == "__main__":
    main()
