#!/usr/bin/env python3

###
# Prints the disassembly of functions (by name or address) from the dtk asm
# output in build/<version>/asm.
#
# Usage:
#   python tools/asm_fn.py <version> xQuatMul__FP6_xQuatP6_xQuatP6_xQuat 8002CF48 ...
###

import re
import sys
from pathlib import Path


def main() -> None:
    version = sys.argv[1]
    wanted = sys.argv[2:]
    asm_dir = Path("build") / version / "asm"
    for path in sorted(asm_dir.rglob("*.s")):
        lines = path.read_text().splitlines()
        i = 0
        while i < len(lines):
            m = re.match(r"^\.fn (\S+),", lines[i])
            if m:
                name = m.group(1)
                addr_line = lines[i + 1] if i + 1 < len(lines) else ""
                am = re.match(r"^/\* ([0-9A-F]{8})", addr_line)
                addr = am.group(1) if am else ""
                if any(w == name or w.upper() == addr or (len(w) > 3 and w in name) for w in wanted):
                    j = i
                    while j < len(lines) and not lines[j].startswith(".endfn"):
                        print(lines[j])
                        j += 1
                    print(lines[j] if j < len(lines) else "")
                    print()
                    i = j
            i += 1


if __name__ == "__main__":
    main()
