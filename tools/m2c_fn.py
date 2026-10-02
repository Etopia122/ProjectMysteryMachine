#!/usr/bin/env python3

###
# Runs m2c (https://github.com/matt-kempster/m2c) on one function from the
# dtk-generated asm, as a first C draft for decompiling it.
#
# Uses the ppc-mwcc-c++ target so argument types come from mangled names.
# Extra arguments are passed through to m2c (e.g. --context file.h, --valid-syntax).
#
# Usage:
#   python tools/m2c_fn.py <version> <function name> [m2c args ...]
#
# Install m2c with:
#   python -m pip install git+https://github.com/matt-kempster/m2c.git
###

import subprocess
import sys
from pathlib import Path


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
    cmd = [sys.executable, "-m", "m2c.main", "-t", "ppc-mwcc-c++", "--function", func, *extra, str(matches[0])]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == "__main__":
    main()
