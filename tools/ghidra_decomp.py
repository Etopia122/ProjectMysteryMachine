#!/usr/bin/env python3

###
# Decompiles functions with headless Ghidra, as first C drafts for matching.
#
# The first run imports and analyzes build/GIHE78/main.elf (our linked ELF:
# byte-identical to the DOL and carrying every symbol from symbols.txt) into
# a Ghidra project under build/ghidra. Later runs reuse that analysis.
#
# Usage:
#   python tools/ghidra_decomp.py <out dir> <function name or list file> ...
#   python tools/ghidra_decomp.py --reimport ...   (re-analyze a fresh ELF)
#
# Ghidra is found through GHIDRA_INSTALL_DIR, or the default
# C:/Users/Ethan/Tools/ghidra/ghidra_12.1.4_PUBLIC.
###

import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GHIDRA = os.environ.get("GHIDRA_INSTALL_DIR", r"C:\Users\Ethan\Tools\ghidra\ghidra_12.1.4_PUBLIC")
PROJ_DIR = os.path.join(ROOT, "build", "ghidra")
PROJ_NAME = "scoob"
ELF = os.path.join(ROOT, "build", "GIHE78", "main.elf")
SCRIPTS = os.path.join(ROOT, "tools", "ghidra")
LANGUAGE = "PowerPC:BE:32:default"


def headless():
    exe = "analyzeHeadless.bat" if os.name == "nt" else "analyzeHeadless"
    return os.path.join(GHIDRA, "support", exe)


def main() -> None:
    args = sys.argv[1:]
    reimport = False
    if args and args[0] == "--reimport":
        reimport = True
        args = args[1:]
    if len(args) < 2:
        sys.exit(__doc__ if __doc__ else "usage: ghidra_decomp.py <out dir> <names|list files>")
    out_dir = os.path.abspath(args[0])

    names = []
    for a in args[1:]:
        if os.path.isfile(a):
            names += [l.strip() for l in open(a) if l.strip() and not l.startswith("#")]
        else:
            names.append(a)
    fd, list_file = tempfile.mkstemp(suffix=".txt")
    with os.fdopen(fd, "w") as f:
        f.write("\n".join(names) + "\n")

    have_project = os.path.exists(os.path.join(PROJ_DIR, PROJ_NAME + ".gpr"))
    if reimport and have_project:
        shutil.rmtree(PROJ_DIR)
        have_project = False
    os.makedirs(PROJ_DIR, exist_ok=True)

    cmd = [headless(), PROJ_DIR, PROJ_NAME]
    if have_project:
        cmd += ["-process", "main.elf", "-noanalysis"]
    else:
        cmd += ["-import", ELF, "-processor", LANGUAGE]
    cmd += ["-scriptPath", SCRIPTS, "-postScript", "DecompileList.java", list_file, out_dir]
    r = subprocess.run(cmd)
    os.remove(list_file)
    sys.exit(r.returncode)


if __name__ == "__main__":
    main()
