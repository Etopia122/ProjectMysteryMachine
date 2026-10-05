#!/usr/bin/env python3
# Side-by-side diff of one function using objdiff-cli (uses the unit's real
# build, so it works for SDK/library units that tools/diff_fn.sh cannot compile).
# usage: tools/objdiff_fn.py <unit name, e.g. main/dolphin/src/dvd/dvdfs> <symbol>
# Left column: the original (target); right column: our build.
import json
import subprocess
import sys
from pathlib import Path

unit, sym = sys.argv[1], sys.argv[2]
out = Path("build/tmp/objdiff_fn.json")
subprocess.run([str(Path("build/tools/objdiff-cli.exe").resolve()), "diff", "-p", ".", "-u", unit, "-o", str(out), "--format", "json", sym],
               capture_output=True)
d = json.loads(out.read_text())


def fmt(ins):
    if not ins or "instruction" not in ins:
        return ""
    return ins["instruction"].get("formatted", "")


def find(side):
    for s in d[side].get("symbols", []):
        if s.get("name") == sym:
            return s
    return None


left, right = find("left"), find("right")
li = left.get("instructions", []) if left else []
ri = right.get("instructions", []) if right else []
for i in range(max(len(li), len(ri))):
    a = li[i] if i < len(li) else {}
    b = ri[i] if i < len(ri) else {}
    mark = "!!" if a.get("diff_kind") or b.get("diff_kind") else "  "
    print("%s %-40s | %s" % (mark, fmt(a), fmt(b)))
