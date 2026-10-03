#!/usr/bin/env python3

###
# Runs m2c (https://github.com/matt-kempster/m2c) on one function from the
# dtk-generated asm, as a first C draft for decompiling it.
#
# Uses the ppc-mwcc-c++ target so argument types come from mangled names.
# Extra arguments are passed through to m2c (e.g. --context file.h, --valid-syntax).
#
# Switch jump tables are found automatically: dtk writes their entries as
# `function+offset`, so the table is rewritten with local labels and the
# labels are inserted into a temporary copy of the function's asm.
#
# Usage:
#   python tools/m2c_fn.py <version> <function name> [m2c args ...]
#
# Install m2c with:
#   python -m pip install git+https://github.com/matt-kempster/m2c.git
###

import re
import subprocess
import sys
import tempfile
from pathlib import Path


def find_jumptables(asm_root: Path, func: str, text: str):
    """Returns (labels to insert, rewritten jump table asm) for func."""
    start = text.index(f".fn {func},")
    end = text.index(f".endfn {func}", start)
    body = text[start:end]
    m = re.search(r"/\* ([0-9A-F]{8}) ", body)
    base = int(m.group(1), 16)
    names = sorted(set(re.findall(r"(jumptable_[0-9A-F]{8})@ha", body)))
    targets = set()
    tables = []
    for name in names:
        for p in asm_root.rglob("*.s"):
            data = p.read_text(errors="ignore")
            i = data.find(f".obj {name},")
            if i < 0:
                continue
            j = data.index(f".endobj {name}", i)
            entries = re.findall(r"\.4byte \S+\+0x([0-9A-F]+)", data[i:j])
            labels = ["%08X" % (base + int(off, 16)) for off in entries]
            targets.update(labels)
            tables.append(
                f".obj {name}, global\n"
                + "".join(f"\t.4byte .L_{l}\n" for l in labels)
                + f".endobj {name}\n"
            )
            break
    return targets, ".section .data\n" + "".join(tables) if tables else None


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(__doc__ or "usage: m2c_fn.py <version> <function> [m2c args]")
    version, func = sys.argv[1], sys.argv[2]
    extra = sys.argv[3:]
    asm_root = Path("build") / version / "asm"
    needle = f".fn {func},"
    matches = [p for p in asm_root.rglob("*.s") if needle in p.read_text(errors="ignore")]
    if not matches:
        sys.exit(f"{func} not found under {asm_root} (run ninja first?)")
    text = matches[0].read_text(errors="ignore")
    targets, table = find_jumptables(asm_root, func, text)
    files = [str(matches[0])]
    tmp = None
    if table:
        tmp = tempfile.TemporaryDirectory()
        out = []
        for line in text.splitlines(keepends=True):
            m = re.match(r"/\* ([0-9A-F]{8}) ", line)
            if m and m.group(1) in targets and not (out and out[-1].startswith(f".L_{m.group(1)}:")):
                out.append(f".L_{m.group(1)}:\n")
            out.append(line)
        asm = Path(tmp.name) / "func.s"
        asm.write_text("".join(out))
        jt = Path(tmp.name) / "jumptables.s"
        jt.write_text(table)
        files = [str(asm), str(jt)]
    cmd = [sys.executable, "-m", "m2c.main", "-t", "ppc-mwcc-c++", "--function", func, *extra, *files]
    rc = subprocess.run(cmd).returncode
    if tmp:
        tmp.cleanup()
    sys.exit(rc)


if __name__ == "__main__":
    main()
