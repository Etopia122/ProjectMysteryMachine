#!/usr/bin/env python3

###
# Prints what's needed to start decompiling a TU: the PS2 DWARF signatures
# (in PS2 order), the GameCube functions in the TU's .text split (DOL order),
# data references with neighbour bounds (tools/data_splits.py), and compact
# disassembly.
#
# Usage:
#   python tools/tu_info.py <version> <unit, e.g. Game/zTimer.cpp> [--no-asm]
###

import json
import re
import subprocess
import sys
from pathlib import Path

RE_FUNC = re.compile(r"^(\S+) = \.text:0x([0-9A-F]+); // type:function size:0x([0-9A-F]+)")
RE_ASM = re.compile(r"^/\* [0-9A-F]{8} [0-9A-F]{8}  (?:[0-9A-F]{2} ){4}\*/\t")


def main() -> None:
    version, unit = sys.argv[1], sys.argv[2]
    show_asm = "--no-asm" not in sys.argv
    base = unit.split("/")[-1]

    ps2_path = Path("build/symbol_import/ps2.json")
    if ps2_path.exists():
        for u in json.loads(ps2_path.read_text()):
            if u["name"].replace("\\", "/").split("/")[-1].lower() == base.lower():
                print(f"== PS2 {base} ({len(u['functions'])} functions, PS2 order)")
                for f in u["functions"]:
                    print(f"  {f['end'] - f['start']:5X}  {f['signature']}")
                globs = [g for g in u["globals"] if not g["is_function"] and g["address"]]
                if globs:
                    print("  globals:")
                    for g in globs:
                        print(f"    {g['size']:5X}  {g['decl']}")

    splits = Path("config") / version / "splits.txt"
    text = None
    cur = None
    for line in splits.read_text().splitlines():
        if line.startswith(unit + ":"):
            cur = unit
            continue
        if cur and line.startswith("\t.text"):
            m = re.search(r"start:0x([0-9A-F]+) end:0x([0-9A-F]+)", line)
            text = (int(m.group(1), 16), int(m.group(2), 16))
            break
        if cur and not line.startswith("\t"):
            break
    if text is None:
        sys.exit(f"{unit}: no .text split")

    print(f"\n== GC {unit} .text {text[0]:08X}-{text[1]:08X} (DOL order)")
    names = []
    for line in (Path("config") / version / "symbols.txt").read_text().splitlines():
        m = RE_FUNC.match(line)
        if m and text[0] <= int(m.group(2), 16) < text[1]:
            names.append((int(m.group(2), 16), m.group(1), int(m.group(3), 16)))
    for addr, name, size in sorted(names):
        print(f"  {addr:08X} {size:5X}  {name}")

    print()
    subprocess.run([sys.executable, "tools/data_splits.py", version, unit])

    if show_asm:
        asm_dir = Path("build") / version / "asm"
        want = {n for _, n, _ in names}
        printed = set()
        for path in sorted(asm_dir.rglob("*.s")):
            lines = path.read_text().splitlines()
            i = 0
            while i < len(lines):
                m = re.match(r"^\.fn (\S+),", lines[i])
                if m and m.group(1) in want and m.group(1) not in printed:
                    printed.add(m.group(1))
                    print(f"\n{lines[i]}")
                    i += 1
                    while i < len(lines) and not lines[i].startswith(".endfn"):
                        print(RE_ASM.sub("    ", lines[i]))
                        i += 1
                i += 1


if __name__ == "__main__":
    main()
