#!/usr/bin/env python3
"""Hill-climbs the order of a block of local declarations to improve a
function's match (MWCC's register allocation often follows declaration order).

Mark the block in the source with lines containing PERM_BEGIN and PERM_END
(e.g. `// PERM_BEGIN`). Each line in between is one movable declaration.
The best order found is written back to the file (markers kept).

Usage:
  python tools/perm_decls.py <src file> <unit> <function substring> [--iters N] [--seed S]
"""
import argparse
import random
import re
import subprocess
import sys
from pathlib import Path


def score(unit: str, fn: str) -> float:
    out = subprocess.run(
        [sys.executable, "tools/check_unit.py", "GIHE78", unit], capture_output=True, text=True
    ).stdout
    for line in out.splitlines():
        if fn in line:
            m = re.match(r"\s*([\d.]+)%", line)
            if m:
                return float(m.group(1))
    return -1.0


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("unit")
    ap.add_argument("fn")
    ap.add_argument("--iters", type=int, default=200)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()
    random.seed(args.seed)

    path = Path(args.src)
    text = path.read_text()
    lines = text.splitlines(keepends=True)
    b = next(i for i, l in enumerate(lines) if "PERM_BEGIN" in l)
    e = next(i for i, l in enumerate(lines) if "PERM_END" in l)
    head, decls, tail = lines[: b + 1], lines[b + 1 : e], lines[e:]

    def write(order):
        path.write_text("".join(head + order + tail))

    best = list(decls)
    write(best)
    best_score = score(args.unit, args.fn)
    print(f"start {best_score:.2f}%", flush=True)
    try:
        for it in range(args.iters):
            cand = list(best)
            kind = random.random()
            i, j = random.sample(range(len(cand)), 2)
            if kind < 0.5:
                cand[i], cand[j] = cand[j], cand[i]
            else:
                cand.insert(j, cand.pop(i))
            write(cand)
            s = score(args.unit, args.fn)
            if s > best_score:
                best, best_score = cand, s
                print(f"[{it}] {best_score:.2f}%", flush=True)
                if best_score >= 100.0:
                    break
    finally:
        write(best)
    print(f"best {best_score:.2f}%")
    print("".join(best))


if __name__ == "__main__":
    main()
