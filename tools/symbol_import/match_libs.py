#!/usr/bin/env python3

###
# Names library functions (SDK, MSL, Runtime, MetroTRK, Bink, RenderWare) by
# locating each library TU of a reference GameCube decomp (dtk-style
# symbols.txt + splits.txt) in our .text with local alignment on function
# sizes. Libraries are prebuilt, so function order within a TU matches the
# reference; only link order and some sizes (library versions) differ.
#
# TU placements are accepted greedily by score without overlap; within a
# placement, functions are renamed when sizes match closely (or exactly for
# small functions), or when they fill a one-to-one gap between such matches.
# Only auto-generated names (fn_XXXXXXXX) are replaced, and names already
# present in symbols.txt are never duplicated.
#
# Usage:
#   python tools/symbol_import/match_libs.py <version> gc.json <ref symbols.txt> <ref splits.txt>
#       <start> <end> <report.txt> [unit prefix to skip ...]
###

import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

sys.path.insert(0, str(Path(__file__).parent))
from bfbb_names import functions_by_unit  # noqa: E402

GAP_REF = -1.0
GAP_GC = -1.0
MIN_TU_SCORE = 5.0
SIZE_TOL = (0.93, 1.075)

RE_FN_LINE = re.compile(r"^(fn_[0-9A-F]{8}) = (\.text:0x[0-9A-F]+; // type:function.*)$")
RE_NAME = re.compile(r"^(\S+) = ")


def pair_score(p: int, g: int) -> float:
    if p == g:
        return 2.5 if p > 8 else 1.0
    if p <= 8 or g <= 8:
        return -1.5
    return max(-2.5, 2.0 - 8.0 * abs(math.log(g / p)))


def local_align(ref: List[int], gc: List[int]) -> Tuple[float, List[Tuple[int, int]]]:
    n, m = len(ref), len(gc)
    prev = [0.0] * (m + 1)
    trace = [bytearray(m + 1)]
    best, bi, bj = 0.0, 0, 0
    for i in range(1, n + 1):
        cur = [0.0] * (m + 1)
        tr = bytearray(m + 1)
        p = ref[i - 1]
        for j in range(1, m + 1):
            v, t = 0.0, 0
            d = prev[j - 1] + pair_score(p, gc[j - 1])
            if d > v:
                v, t = d, 1
            u = prev[j] + GAP_REF
            if u > v:
                v, t = u, 2
            l = cur[j - 1] + GAP_GC
            if l > v:
                v, t = l, 3
            cur[j], tr[j] = v, t
            if v > best:
                best, bi, bj = v, i, j
        trace.append(tr)
        prev = cur
    pairs = []
    i, j = bi, bj
    while i > 0 and j > 0 and trace[i][j]:
        t = trace[i][j]
        if t == 1:
            pairs.append((i - 1, j - 1))
            i, j = i - 1, j - 1
        elif t == 2:
            i -= 1
        else:
            j -= 1
    pairs.reverse()
    return best, pairs


def size_ok(r: int, g: int) -> bool:
    if r == g:
        return True
    if r < 0x20 or g < 0x20:
        return False
    return SIZE_TOL[0] <= g / r <= SIZE_TOL[1]


def main() -> None:
    version = sys.argv[1]
    gc = json.loads(Path(sys.argv[2]).read_text())
    ref = functions_by_unit(Path(sys.argv[3]), Path(sys.argv[4]), full_path=True)
    start, end = int(sys.argv[5], 16), int(sys.argv[6], 16)
    report_path = Path(sys.argv[7])
    dry_run = "--dry-run" in sys.argv[8:]
    skip = tuple(a for a in sys.argv[8:] if a != "--dry-run")

    funcs = [f for f in gc["functions"] if f["section"] == ".text" and start <= f["address"] < end]
    sizes = [f["size"] for f in funcs]

    candidates = []
    for unit, rfuncs in ref.items():
        if skip and unit.startswith(skip):
            continue
        rsizes = [f["size"] for f in rfuncs]
        score, pairs = local_align(rsizes, sizes)
        if score >= MIN_TU_SCORE and len(pairs) >= max(1, len(rfuncs) // 3):
            candidates.append((score, unit, pairs))
    candidates.sort(key=lambda c: -c[0])

    regions_path = Path("config") / version / "library_regions.json"
    regions = []
    if regions_path.exists():
        for r in json.loads(regions_path.read_text())["regions"]:
            regions.append((int(r["start"], 16), int(r["end"], 16), tuple(r["prefixes"])))

    def plausible(unit: str, addr: int) -> bool:
        for lo, hi, prefixes in regions:
            if lo <= addr < hi:
                return bool(prefixes) and unit.startswith(prefixes)
        return not regions

    taken: List[Tuple[int, int]] = []
    placements = []
    for score, unit, pairs in candidates:
        lo, hi = pairs[0][1], pairs[-1][1]
        if any(not (hi < a or lo > b) for a, b in taken):
            continue
        if not (plausible(unit, funcs[lo]["address"]) and plausible(unit, funcs[hi]["address"])):
            continue
        taken.append((lo, hi))
        placements.append((lo, unit, score, pairs))
    placements.sort()

    names: Dict[int, str] = {}
    report = []
    for lo, unit, score, pairs in placements:
        rfuncs = ref[unit]
        ok = [size_ok(rfuncs[i]["size"], funcs[j]["size"]) for i, j in pairs]
        anchors = [k for k, v in enumerate(ok) if v]
        for a, b in zip(anchors, anchors[1:]):
            (ia, ja), (ib, jb) = pairs[a], pairs[b]
            if ib - ia == jb - ja == b - a:
                for k in range(a + 1, b):
                    ok[k] = True
        report.append(
            f"## {funcs[pairs[0][1]]['address']:08X}-{funcs[pairs[-1][1]]['address'] + funcs[pairs[-1][1]]['size']:08X} {unit} score {score:.1f} ({len(pairs)}/{len(rfuncs)})"
        )
        for (i, j), good in zip(pairs, ok):
            r, f = rfuncs[i], funcs[j]
            report.append(
                f"{'ACCEPT' if good else 'reject'} {f['address']:08X} gc:{f['size']:5X} ref:{r['size']:5X} {r['mangled']}"
            )
            if good and not r["mangled"].startswith(("fn_", "lbl_")):
                names[f["address"]] = r["mangled"]

    symbols_path = Path("config") / version / "symbols.txt"
    lines = symbols_path.read_text().splitlines()
    existing = {m.group(1) for l in lines if (m := RE_NAME.match(l))}
    renamed = 0
    out = []
    for line in lines:
        m = RE_FN_LINE.match(line)
        if m:
            new = names.get(int(m.group(1)[3:], 16))
            if new and new not in existing:
                line = f"{new} = {m.group(2)}"
                existing.add(new)
                renamed += 1
        out.append(line)
    if not dry_run:
        symbols_path.write_text("\n".join(out) + "\n")
    report_path.write_text("\n".join(report) + "\n")
    print(f"placed {len(placements)} TUs; accepted {len(names)} functions; renamed {renamed}")


if __name__ == "__main__":
    main()
