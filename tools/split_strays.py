#!/usr/bin/env python3
# For every configured unit with a compiled object, list functions that the
# object defines and the DOL has (paired by name via symbols.txt) but that lie
# OUTSIDE the unit's .text split: candidates for extending a split over an
# unsplit gap. Marks each with whether its bytes already match (masked).
# usage: tools/split_strays.py [unit substring...]
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from place_ref_objects import Dol, object_text  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
dol = Dol(ROOT / "orig" / "GIHE78" / "sys" / "main.dol")
syms = {}
for line in open(ROOT / "config" / "GIHE78" / "symbols.txt"):
    m = re.match(r"(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)", line)
    if m:
        syms.setdefault(m.group(1), []).append((int(m.group(2), 16), int(m.group(3), 16)))

splits = {}
unit = None
for line in open(ROOT / "config" / "GIHE78" / "splits.txt"):
    m = re.match(r"^(\S.*):\s*$", line)
    if m:
        unit = m.group(1)
        continue
    m = re.match(r"^\s+\.text\s+start:0x(\w+) end:0x(\w+)", line)
    if m and unit:
        splits[unit] = (int(m.group(1), 16), int(m.group(2), 16))

want = sys.argv[1:]
for u, (lo, hi) in sorted(splits.items(), key=lambda x: x[1]):
    if want and not any(w in u for w in want):
        continue
    obj = ROOT / "build" / "GIHE78" / "src" / (u.rsplit(".", 1)[0] + ".o")
    if not obj.exists():
        continue
    text, masks, funcs = object_text(obj)
    out = []
    for off, size, name in funcs:
        for addr, dsize in syms.get(name, []):
            if lo <= addr < hi:
                break
        else:
            # pick the candidate closest to the split
            cands = syms.get(name, [])
            if not cands:
                continue
            addr, dsize = min(cands, key=lambda c: min(abs(c[0] - lo), abs(c[0] - hi)))
            if min(abs(addr - lo), abs(addr - hi)) > 0x4000:
                continue
            hay = dol.region(addr, addr + dsize)
            n = min(size, dsize) // 4
            eq = sum(
                1
                for i in range(n)
                if (struct.unpack_from(">I", text, off + i * 4)[0] & (~masks.get(off + i * 4, 0) & 0xFFFFFFFF))
                == (struct.unpack_from(">I", hay, i * 4)[0] & (~masks.get(off + i * 4, 0) & 0xFFFFFFFF))
            )
            tag = "EXACT" if size == dsize and eq == n else "diff"
            out.append("   %08X %5X %-6s %s" % (addr, dsize, tag, name))
    if out:
        print("%s  [%08X-%08X]" % (u, lo, hi))
        print("\n".join(sorted(out)))
