#!/usr/bin/env python3

###
# Tests units one at a time: marks each given object Matching (everything else
# as currently configured), rebuilds, and reports whether the DOL hash still
# matches. configure.py is restored afterwards.
#
# Usage:
#   python tools/bisect_units.py <object path> [<object path> ...]
#   e.g. python tools/bisect_units.py dolphin/src/os/OSResetSW.c Game/zTimer.cpp
###

import subprocess
import sys
from pathlib import Path


def build_result(version: str = "GIHE78") -> str:
    ok = Path("build") / version / "ok"
    if ok.exists():
        ok.unlink()
    subprocess.run([sys.executable, "configure.py"], capture_output=True)
    proc = subprocess.run(["ninja"], capture_output=True, text=True)
    if ok.exists():
        return "MATCH   "
    if "Linker Error" in proc.stdout or "mwldeppc" in proc.stdout and "FAILED" in proc.stdout:
        undefined = [l.strip() for l in proc.stdout.splitlines() if "undefined:" in l]
        return "LINKFAIL" + (f" ({undefined[0]})" if undefined else "")
    return "MISMATCH"


def main() -> None:
    units = sys.argv[1:]
    cfg = Path("configure.py")
    original = cfg.read_text()
    try:
        for unit in units:
            needle = f'Object(NonMatching, "{unit}")'
            if needle not in original:
                print(f"SKIP     {unit} (not NonMatching in configure.py)")
                continue
            cfg.write_text(original.replace(needle, f'Object(Matching, "{unit}")'))
            print(f"{build_result()} {unit}", flush=True)
    finally:
        cfg.write_text(original)
        subprocess.run([sys.executable, "configure.py"], capture_output=True)


if __name__ == "__main__":
    main()
