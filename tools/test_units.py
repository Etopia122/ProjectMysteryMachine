#!/usr/bin/env python3

###
# Tests units one at a time: switches each from NonMatching to Matching in
# configure.py, rebuilds, and reports whether the DOL hash still matches.
# configure.py is restored after every test.
#
# Unlike bisect_units.py this handles Object(...) entries with extra
# arguments and kills hung builds (the linker can spin forever on some bad
# splits).
#
# Usage:
#   python tools/test_units.py <unit path> [<unit path> ...] [--timeout 300] [--keep]
#
# --keep leaves every unit that matched switched to Matching.
###

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build(timeout: int, version: str = "GIHE78") -> str:
    ok = ROOT / "build" / version / "ok"
    if ok.exists():
        ok.unlink()
    subprocess.run([sys.executable, "configure.py"], capture_output=True, cwd=ROOT)
    log_path = ROOT / "build" / "tmp" / "test_units.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w") as log:
        proc = subprocess.Popen(["ninja"], stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
        try:
            proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
            subprocess.run(["taskkill", "/F", "/IM", "mwldeppc.exe"], capture_output=True)
            return "HANG"
    if ok.exists():
        return "MATCH"
    out = log_path.read_text(errors="ignore")
    errs = [l.strip() for l in out.splitlines() if "undefined:" in l or "multiply-defined:" in l]
    if errs:
        return "LINKFAIL " + errs[0]
    if "error" in out.lower():
        bad = [l.strip() for l in out.splitlines() if "rror" in l][:1]
        return "ERROR " + (bad[0] if bad else "")
    return "MISMATCH"


def main() -> None:
    args = sys.argv[1:]
    timeout = 300
    if "--timeout" in args:
        i = args.index("--timeout")
        timeout = int(args[i + 1])
        del args[i : i + 2]
    keep = "--keep" in args
    units = [a for a in args if a != "--keep"]
    cfg = ROOT / "configure.py"
    for unit in units:
        original = cfg.read_text()
        pat = re.compile(r'Object\(NonMatching,(\s*)"' + re.escape(unit) + '"')
        if not pat.search(original):
            print(f"SKIP      {unit} (not NonMatching)")
            continue
        cfg.write_text(pat.sub(lambda m: f'Object(Matching,{m.group(1)}"{unit}"', original), newline="\n")
        result = build(timeout)
        print(f"{result:9} {unit}", flush=True)
        if not (keep and result == "MATCH"):
            cfg.write_text(original, newline="\n")
    build(timeout)


if __name__ == "__main__":
    main()
