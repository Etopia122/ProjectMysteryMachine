#!/usr/bin/env python3

###
# Renames symbols in config/<version>/symbols.txt by address.
#
# Usage:
#   python tools/rename.py <version> 0x80134384=CARDProbeEx 80134508=DoMount ...
###

import re
import sys
from pathlib import Path

RE_SYMBOL = re.compile(r"^(\S+) = (\.\w+:0x([0-9A-Fa-f]+);.*)$")


def main() -> None:
    version = sys.argv[1]
    renames = {}
    for arg in sys.argv[2:]:
        addr, name = arg.strip().split("=", 1)
        renames[int(addr, 16)] = name
    path = Path("config") / version / "symbols.txt"
    lines = path.read_text().splitlines()
    existing = {RE_SYMBOL.match(l).group(1) for l in lines if RE_SYMBOL.match(l)}
    out = []
    done = set()
    for line in lines:
        m = RE_SYMBOL.match(line)
        if m:
            addr = int(m.group(3), 16)
            new = renames.get(addr)
            if new and new != m.group(1):
                if new in existing:
                    print(f"warning: {new} already defined elsewhere; renaming {addr:08X} anyway")
                print(f"{addr:08X}: {m.group(1)} -> {new}")
                line = f"{new} = {m.group(2)}"
                done.add(addr)
            elif new:
                done.add(addr)
        out.append(line)
    for addr in renames.keys() - done:
        print(f"warning: no symbol at {addr:08X}")
    path.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
