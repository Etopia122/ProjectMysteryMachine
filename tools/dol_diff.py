#!/usr/bin/env python3

###
# Lists address ranges where the built DOL differs from the original, with the
# nearest preceding symbol, to find what broke the hash.
#
# Usage:
#   python tools/dol_diff.py <version> [max ranges]
###

import re
import struct
import sys
from pathlib import Path


def sections(d: bytes):
    offs = struct.unpack(">18I", d[0:72])
    addrs = struct.unpack(">18I", d[72:144])
    sizes = struct.unpack(">18I", d[144:216])
    return [(o, a, s) for o, a, s in zip(offs, addrs, sizes) if s]


def main() -> None:
    version = sys.argv[1]
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else 20
    a = Path("orig", version, "sys", "main.dol").read_bytes()
    b = Path("build", version, "main.dol").read_bytes()
    syms = []
    for line in Path("config", version, "symbols.txt").read_text().splitlines():
        m = re.match(r"^(\S+) = \.\w+:0x([0-9A-Fa-f]+);", line)
        if m:
            syms.append((int(m.group(2), 16), m.group(1)))
    syms.sort()

    def near(addr):
        lo, hi = 0, len(syms)
        while lo < hi:
            mid = (lo + hi) // 2
            if syms[mid][0] <= addr:
                lo = mid + 1
            else:
                hi = mid
        return syms[lo - 1] if lo else (0, "?")

    sa, sb = sections(a), sections(b)
    if [(x[1], x[2]) for x in sa] != [(x[1], x[2]) for x in sb]:
        print("section layout differs:")
        for x, y in zip(sa, sb):
            if (x[1], x[2]) != (y[1], y[2]):
                print(f"  {x[1]:08X}+{x[2]:X} vs {y[1]:08X}+{y[2]:X}")
    found = 0
    for (oa, addr, size), (ob, _, sizeb) in zip(sa, sb):
        da, db = a[oa : oa + size], b[ob : ob + min(size, sizeb)]
        i = 0
        while i < min(len(da), len(db)) and found < limit:
            if da[i] != db[i]:
                j = i
                while j < min(len(da), len(db)) and da[j] != db[j]:
                    j += 1
                s_addr, s_name = near(addr + i)
                print(f"{addr + i:08X}-{addr + j:08X} ({j - i:X} bytes) in/after {s_name} (+{addr + i - s_addr:X})")
                found += 1
                i = j
            i += 1
    if not found:
        print("no differences in common section bytes")


if __name__ == "__main__":
    main()
