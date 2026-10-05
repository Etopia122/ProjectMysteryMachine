#!/usr/bin/env python3
# Per-function comparison of a compiled reference object (from
# tools/place_ref_objects.py, under build/tmp/ref_objects) against the DOL.
# Functions are paired by name through config/GIHE78/symbols.txt; relocated
# fields are masked. Prints, per function: exact / same size / size delta and
# the share of equal words.
# usage: tools/ref_fncmp.py <object.o> [more objects...]
#        tools/ref_fncmp.py --diff <function> <object.o> [<DOL address>]
import re
import struct
import sys
from pathlib import Path

from capstone import CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN, Cs

sys.path.insert(0, str(Path(__file__).parent))
from place_ref_objects import Dol, object_text  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
dol = Dol(ROOT / "orig" / "GIHE78" / "sys" / "main.dol")
syms = {}
for line in open(ROOT / "config" / "GIHE78" / "symbols.txt"):
    m = re.match(r"(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)", line)
    if m:
        syms[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))

if sys.argv[1] == "--diff":
    fname, path = sys.argv[2], sys.argv[3]
    text, masks, funcs = object_text(Path(path))
    off, size = next((o, s) for o, s, n in funcs if n == fname)
    if len(sys.argv) > 4:
        addr = int(sys.argv[4], 16)
        dsize = syms.get(fname, (0, size))[1] if fname in syms and syms[fname][0] == addr else size
    else:
        addr, dsize = syms[fname]
    hay = dol.region(addr, addr + max(dsize, size))
    md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)

    def dis(code, base):
        out = []
        for i in range(0, len(code), 4):
            ins = next(md.disasm(code[i : i + 4], base + i), None)
            out.append(f"{ins.mnemonic} {ins.op_str}" if ins else ".word %08x" % struct.unpack_from(">I", code, i))
        return out

    a, b = dis(text[off : off + size], 0), dis(hay[:dsize], 0)
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else ""
        y = b[i] if i < len(b) else ""
        wa = struct.unpack_from(">I", text, off + i * 4)[0] if i * 4 < size else None
        wb = struct.unpack_from(">I", hay, i * 4)[0] if i * 4 < dsize else None
        k = ~masks.get(off + i * 4, 0) & 0xFFFFFFFF
        same = wa is not None and wb is not None and (wa & k) == (wb & k)
        print("%s %4X %-36s | %s" % ("  " if same else "!!", i * 4, x, y))
    sys.exit(0)

tot = good = 0
for path in sys.argv[1:]:
    text, masks, funcs = object_text(Path(path))
    print("==", path)
    for off, size, name in funcs:
        if name not in syms:
            print("   %-40s %5X  (not in DOL symbols)" % (name[:40], size))
            continue
        addr, dsize = syms[name]
        hay = dol.region(addr, addr + dsize)
        n = min(size, dsize) // 4
        eq = 0
        for i in range(n):
            a = struct.unpack_from(">I", text, off + i * 4)[0]
            b = struct.unpack_from(">I", hay, i * 4)[0]
            k = ~masks.get(off + i * 4, 0) & 0xFFFFFFFF
            eq += (a & k) == (b & k)
        tag = "EXACT" if size == dsize and eq == n else ("same size" if size == dsize else "size %+d" % (size - dsize))
        tot += dsize
        if tag == "EXACT":
            good += dsize
        print("   %-40s %08X %5X %-10s %5.1f%%" % (name[:40], addr, dsize, tag, 100.0 * eq / max(1, max(size, dsize) // 4)))
print("exact bytes %d / %d" % (good, tot))
