#!/usr/bin/env python3

###
# Runs `dtk dol split` and, when it fails because a newly named (signature-
# matched) object overlaps auto-generated `lbl_` symbols, removes those
# auto-generated symbols from symbols.txt and retries.
#
# Usage:
#   python tools/symbol_import/resolve_overlaps.py <version>
###

import re
import subprocess
import sys
from pathlib import Path

RE_OVERLAP = re.compile(
    r"Symbol (\S+) \(0x([0-9A-F]+)\.\.0x([0-9A-F]+)\) overlaps with symbol (\S+) \(0x([0-9A-F]+)\.\.0x([0-9A-F]+)"
)
RE_SYMBOL = re.compile(r"^(\S+) = \.\w+:0x([0-9A-F]+);")
AUTO = ("lbl_", "gap_", "jumptable_")


def main() -> None:
    version = sys.argv[1]
    config = Path("config") / version / "config.yml"
    symbols = Path("config") / version / "symbols.txt"
    dtk = Path("build/tools/dtk.exe" if sys.platform == "win32" else "build/tools/dtk")
    for attempt in range(50):
        proc = subprocess.run(
            [str(dtk), "dol", "split", str(config), str(Path("build") / version)],
            capture_output=True,
            text=True,
        )
        if proc.returncode == 0:
            print(f"dtk split succeeded after {attempt} fix(es)")
            return
        m = RE_OVERLAP.search(proc.stderr + proc.stdout)
        if not m:
            sys.exit(proc.stderr + proc.stdout)
        a_name, a_lo, a_hi, b_name = m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)
        named, lo, hi = (a_name, a_lo, a_hi) if not a_name.startswith(AUTO) else (b_name, int(m.group(5), 16), int(m.group(6), 16))
        if named.startswith(AUTO):
            sys.exit(f"overlap between two auto symbols, fix manually:\n{m.group(0)}")
        lines = symbols.read_text().splitlines()
        kept = []
        removed = []
        for line in lines:
            sm = RE_SYMBOL.match(line)
            if sm and sm.group(1).startswith(AUTO) and lo <= int(sm.group(2), 16) < hi:
                removed.append(sm.group(1))
                continue
            kept.append(line)
        if not removed:
            sys.exit(f"nothing to remove for:\n{m.group(0)}")
        symbols.write_text("\n".join(kept) + "\n")
        print(f"{named}: removed {', '.join(removed)}")
    sys.exit("too many overlap fixes")


if __name__ == "__main__":
    main()
