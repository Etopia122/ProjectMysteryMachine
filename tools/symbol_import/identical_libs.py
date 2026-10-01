#!/usr/bin/env python3

###
# Lists library TUs whose every function matches the reference decomp's
# function sizes exactly (from a match_libs.py report), together with the
# reference project's matching status for that TU. These are the best
# candidates for reusing the reference's (already matching) source.
#
# Usage:
#   python tools/symbol_import/identical_libs.py <report_libs.txt> <ref configure.py>
###

import re
import sys
from collections import Counter
from pathlib import Path

RE_HEADER = re.compile(r"^## (\S+) (\S+) score [\d.]+ \((\d+)/(\d+)\)")
RE_ROW = re.compile(r"gc:\s*([0-9A-F]+) ref:\s*([0-9A-F]+)")
RE_OBJECT = re.compile(r'Object\(\s*(\w+)\s*,\s*"([^"]+)"')


def main() -> None:
    report = Path(sys.argv[1]).read_text().splitlines()
    status = {m.group(2): m.group(1) for m in RE_OBJECT.finditer(Path(sys.argv[2]).read_text())}

    tus = {}
    cur = None
    for line in report:
        m = RE_HEADER.match(line)
        if m:
            cur = m.group(2)
            tus[cur] = {"range": m.group(1), "ref_n": int(m.group(4)), "n": 0, "exact": 0}
            continue
        m = RE_ROW.search(line)
        if cur and m:
            tus[cur]["n"] += 1
            tus[cur]["exact"] += m.group(1) == m.group(2)

    rows = [
        (status.get(u, "?"), t["range"], t["n"], u)
        for u, t in tus.items()
        if t["exact"] == t["n"] == t["ref_n"]
    ]
    print("identical TUs by reference status:", dict(Counter(r[0] for r in rows)))
    for st, rng, n, u in sorted(rows):
        print(f"{st:12} {rng} {n:3}  {u}")


if __name__ == "__main__":
    main()
