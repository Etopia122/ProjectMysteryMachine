#!/usr/bin/env python3

###
# Builds a per-function table of the GameCube DOL from decomp-toolkit output.
#
# For every .text function: address, size, callees, and the data objects it
# references (with the section of each). String pools are decoded so a
# function's strings can be inspected.
#
# Usage:
#   python tools/symbol_import/analyze_gc.py <version> <out.json>
###

import json
import re
import struct
import sys
from bisect import bisect_right
from pathlib import Path
from typing import Dict, List, Tuple

RE_SYMBOL = re.compile(
    r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // type:(\w+)(?: size:0x([0-9A-Fa-f]+))?"
)
RE_FN = re.compile(r"^\.fn (\S+),")
RE_ENDFN = re.compile(r"^\.endfn (\S+)")
RE_INSN = re.compile(r"^/\* ([0-9A-F]{8}) [0-9A-F]{8}  (?:[0-9A-F]{2} ){4}\*/\t(\S+)\s*(.*)$")
RE_REF = re.compile(r"((?:fn|lbl)_([0-9A-F]{8})|[A-Za-z_@$][\w@$]*)(?:\+0x[0-9A-F]+)?@(ha|l|sda21)")
RE_BL = re.compile(r"^(?:bl|b)\s+(\S+)$")


class Dol:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        d = self.data
        self.offsets = struct.unpack(">18I", d[0:72])
        self.addrs = struct.unpack(">18I", d[72:144])
        self.sizes = struct.unpack(">18I", d[144:216])

    def read(self, addr: int, size: int) -> bytes:
        for o, a, s in zip(self.offsets, self.addrs, self.sizes):
            if s and a <= addr < a + s:
                start = o + addr - a
                return self.data[start : start + min(size, a + s - addr)]
        return b""


def decode_strings(blob: bytes, limit: int = 8) -> List[str]:
    out = []
    for part in blob.split(b"\0"):
        if len(part) >= 3 and all(0x20 <= c < 0x7F or c in (9, 10) for c in part):
            out.append(part.decode("ascii"))
            if len(out) >= limit:
                break
    return out


def main() -> None:
    version, out = sys.argv[1], Path(sys.argv[2])
    symbols_path = Path("config") / version / "symbols.txt"
    asm_dir = Path("build") / version / "asm"
    dol = Dol(Path("orig") / version / "sys" / "main.dol")

    names: Dict[str, Tuple[str, int, str, int]] = {}
    objects: List[Tuple[int, int, str, str]] = []
    for line in symbols_path.read_text().splitlines():
        m = RE_SYMBOL.match(line)
        if not m:
            continue
        name, section, addr, kind = m.group(1), m.group(2), int(m.group(3), 16), m.group(4)
        size = int(m.group(5), 16) if m.group(5) else 0
        names[name] = (section, addr, kind, size)
        if kind == "object":
            objects.append((addr, size, section, name))
    objects.sort()
    obj_addrs = [o[0] for o in objects]

    def object_at(addr: int):
        i = bisect_right(obj_addrs, addr) - 1
        if i >= 0 and objects[i][0] <= addr < objects[i][0] + max(objects[i][1], 1):
            return objects[i]
        return None

    funcs = []
    cur = None
    for path in sorted(asm_dir.rglob("*.s")):
        if "_text" not in path.name and "_init" not in path.name:
            continue
        for line in path.read_text().splitlines():
            m = RE_FN.match(line)
            if m:
                name = m.group(1)
                section, addr, _, size = names.get(name, (".text", 0, "function", 0))
                cur = {
                    "name": name,
                    "address": addr,
                    "size": size,
                    "section": section,
                    "calls": [],
                    "refs": {},
                }
                funcs.append(cur)
                continue
            if RE_ENDFN.match(line):
                cur = None
                continue
            if cur is None:
                continue
            m = RE_INSN.match(line)
            if not m:
                continue
            mnemonic, operands = m.group(2), m.group(3)
            if mnemonic in ("bl", "b"):
                target = operands.strip()
                if target in names and names[target][2] == "function" and target != cur["name"]:
                    if target not in cur["calls"]:
                        cur["calls"].append(target)
                continue
            for r in RE_REF.finditer(operands):
                sym = r.group(1)
                if sym in names and names[sym][2] == "object":
                    sec = names[sym][0]
                    cur["refs"].setdefault(sym, sec)

    funcs.sort(key=lambda f: f["address"])

    # Decode strings for referenced string-ish objects
    strings: Dict[str, List[str]] = {}
    for f in funcs:
        for sym, sec in f["refs"].items():
            if sym in strings or sec not in (".rodata", ".data", ".sdata"):
                continue
            _, addr, _, size = names[sym]
            s = decode_strings(dol.read(addr, min(size, 0x400)))
            if s:
                strings[sym] = s

    out.write_text(
        json.dumps(
            {
                "functions": funcs,
                "objects": {n: {"section": v[0], "address": v[1], "size": v[3]} for n, v in names.items() if v[2] == "object"},
                "strings": strings,
            },
            indent=1,
        )
    )
    print(f"{len(funcs)} functions, {len(strings)} string objects")


if __name__ == "__main__":
    main()
