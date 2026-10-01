#!/usr/bin/env python3

###
# Names the GameCube platform layer (i* files) using BFBB's GameCube build
# (GQPE78), which shares Heavy Iron's GameCube platform code.
#
# BFBB's compiler emits functions in source order while Scooby's emits them in
# reverse, so each BFBB TU's function list is reversed before alignment. The
# alignment is global over the i* region, with strict size acceptance, and only
# functions still carrying auto-generated names are renamed.
#
# Usage:
#   python tools/symbol_import/align_bfbb_platform.py <version> gc.json <start> <end> report.txt
###

import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).parent))
from bfbb_names import functions_by_unit  # noqa: E402

GAP = -0.9
SIZE_TOL = (0.9, 1.11)
MIN_SIZE = 0x20

RE_FN_LINE = re.compile(r"^(fn_[0-9A-F]{8}) = (\.text:0x[0-9A-F]+; // type:function.*)$")


def pair_score(p: int, g: int) -> float:
    if p <= 8 or g <= 8:
        return 0.6 if abs(p - g) <= 4 else -1.5
    return max(-2.5, 2.5 - 7.0 * abs(math.log(g / p)))


def main() -> None:
    version = sys.argv[1]
    gc = json.loads(Path(sys.argv[2]).read_text())
    start, end = int(sys.argv[3], 16), int(sys.argv[4], 16)
    report_path = Path(sys.argv[5])

    bf = functions_by_unit(Path("ref/bfbb/symbols.txt"), Path("ref/bfbb/splits.txt"))
    units = sorted((u for u in bf if u.startswith("i")), key=str.lower)
    seq: List[Dict[str, Any]] = []
    for u in units:
        for f in reversed(bf[u]):
            seq.append({"unit": u, **f})

    funcs = [f for f in gc["functions"] if f["section"] == ".text" and start <= f["address"] < end]
    n, m = len(seq), len(funcs)

    prev = [GAP * j for j in range(m + 1)]
    trace = [bytearray([3] * (m + 1))]
    for i in range(1, n + 1):
        cur = [0.0] * (m + 1)
        tr = bytearray(m + 1)
        cur[0] = prev[0] + GAP
        tr[0] = 2
        p = seq[i - 1]["size"]
        for j in range(1, m + 1):
            diag = prev[j - 1] + pair_score(p, funcs[j - 1]["size"])
            up = prev[j] + GAP
            left = cur[j - 1] + GAP
            if diag >= up and diag >= left:
                cur[j], tr[j] = diag, 1
            elif up >= left:
                cur[j], tr[j] = up, 2
            else:
                cur[j], tr[j] = left, 3
        trace.append(tr)
        prev = cur

    pairs = []
    i, j = n, m
    while i > 0 or j > 0:
        t = trace[i][j] if i > 0 else 3
        if t == 1:
            pairs.append((i - 1, j - 1))
            i, j = i - 1, j - 1
        elif t == 2:
            i -= 1
        else:
            j -= 1
    pairs.reverse()

    names: Dict[int, str] = {}
    report = []
    for i, j in pairs:
        b, f = seq[i], funcs[j]
        r = f["size"] / b["size"] if b["size"] else 0
        ok = SIZE_TOL[0] <= r <= SIZE_TOL[1] and f["size"] >= MIN_SIZE
        report.append(
            f"{'ACCEPT' if ok else 'reject'} {f['address']:08X} gc:{f['size']:5X} bfbb:{b['size']:5X} {b['unit']:22} {b['mangled']}"
        )
        if ok and not b["mangled"].startswith("fn_"):
            names[f["address"]] = b["mangled"]

    symbols_path = Path("config") / version / "symbols.txt"
    lines = symbols_path.read_text().splitlines()
    existing = {l.split(" = ")[0] for l in lines if " = " in l}
    renamed = 0
    out = []
    for line in lines:
        mt = RE_FN_LINE.match(line)
        if mt:
            new = names.get(int(mt.group(1)[3:], 16))
            if new and new not in existing:
                line = f"{new} = {mt.group(2)}"
                existing.add(new)
                renamed += 1
        out.append(line)
    symbols_path.write_text("\n".join(out) + "\n")
    report_path.write_text("\n".join(report) + "\n")
    print(f"aligned {len(pairs)} ({n} BFBB i* functions vs {m} GC); accepted {len(names)}; renamed {renamed}")


if __name__ == "__main__":
    main()
