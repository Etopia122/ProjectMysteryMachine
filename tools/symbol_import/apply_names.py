#!/usr/bin/env python3

###
# Applies high-confidence function names from the PS2 -> GC alignment to
# config/<version>/symbols.txt, and writes a report of every decision.
#
# A matched pair is accepted when:
#   - GC and PS2 sizes agree within SIZE_TOL (and the function isn't tiny), or
#   - it lies between two accepted pairs of the same TU with exactly as many
#     GC functions as PS2 functions in between (a one-to-one gap), and its
#     size is within GAP_SIZE_TOL.
# Only auto-generated names (fn_XXXXXXXX) are replaced.
#
# Usage:
#   python tools/symbol_import/apply_names.py <version> ps2.json aligned.json report.txt
###

import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

sys.path.insert(0, str(Path(__file__).parent))
from mangle import MangleError, mangle, parse_signature, split_params, strip_param_name  # noqa: E402

SIZE_TOL = (0.8, 1.25)
GAP_SIZE_TOL = (0.5, 2.0)
MIN_SIZE = 0x18
PLATFORM_SIZE_TOL = (0.92, 1.08)
PLATFORM_MIN_SIZE = 0x30
# Platform files whose GameCube implementation is unrelated to the PS2 one
# (sound, OS, timing, file I/O, pad, memory card, FMV, memory); their names
# come from BFBB's GameCube layer instead (see align_bfbb_platform.py).
PS2_SPECIFIC_PLATFORM = {
    "iSnd.cpp",
    "iSystem.cpp",
    "iTime.cpp",
    "iFile.cpp",
    "iPad.cpp",
    "isavegame.cpp",
    "iFMV.cpp",
    "iMemMgr.cpp",
    "iDebug.cpp",
    "iDraw.cpp",
}

RE_FN_LINE = re.compile(r"^(fn_[0-9A-F]{8}) = (\.text:0x[0-9A-F]+; // type:function.*)$")


def ratio_ok(a: Dict[str, Any], tol) -> bool:
    if a["ps2_size"] == 0:
        return False
    r = a["gc_size"] / a["ps2_size"]
    return tol[0] <= r <= tol[1]


def param_types(sig: str) -> List[str]:
    _, params, _ = parse_signature(sig)
    return [strip_param_name(p) for p in split_params(params)]


def full_signature(sig: str, decls: Dict[str, List[str]]) -> str:
    """Prefer a prototype with the full parameter list when the definition
    dropped unused parameters (dtk omits them from definitions)."""
    qname, _, _ = parse_signature(sig)
    have = param_types(sig)
    best = sig
    for d in decls.get(qname.split("::")[-1], []):
        try:
            dq, _, _ = parse_signature(d)
            if dq != qname:
                continue
            want = param_types(d)
        except (MangleError, ValueError):
            continue
        # Definition params must be an in-order subsequence of the prototype's
        it = iter(want)
        if len(want) > len(have) and all(any(h == w for w in it) for h in have):
            best = d
    return best


def main() -> None:
    version = sys.argv[1]
    ps2 = json.loads(Path(sys.argv[2]).read_text())
    aligned = json.loads(Path(sys.argv[3]).read_text())
    report_path = Path(sys.argv[4])

    decls: Dict[str, List[str]] = {}
    for u in ps2:
        for g in u["globals"]:
            if g["is_function"]:
                d = g["decl"]
                try:
                    qname, _, _ = parse_signature(d)
                except ValueError:
                    continue
                decls.setdefault(qname.split("::")[-1], []).append(d)

    def is_platform(a: Dict[str, Any]) -> bool:
        # i* files are the platform layer, rewritten for GameCube; PS2 names
        # only carry over for near-identical functions.
        return a["unit"].startswith("i")

    accepted = [
        False
        if a["unit"] in PS2_SPECIFIC_PLATFORM
        else (ratio_ok(a, PLATFORM_SIZE_TOL) and a["gc_size"] >= PLATFORM_MIN_SIZE)
        if is_platform(a)
        else (ratio_ok(a, SIZE_TOL) and a["gc_size"] >= MIN_SIZE and a["ps2_size"] >= MIN_SIZE)
        for a in aligned
    ]
    # Fill one-to-one gaps between accepted anchors in the same TU
    anchors = [k for k, ok in enumerate(accepted) if ok]
    for a, b in zip(anchors, anchors[1:]):
        x, y = aligned[a], aligned[b]
        if x["unit"] != y["unit"] or b - a < 2 or is_platform(x):
            continue
        if y["gc_index"] - x["gc_index"] != y["ps2_index"] - x["ps2_index"]:
            continue
        if y["gc_index"] - x["gc_index"] != b - a:
            continue
        for k in range(a + 1, b):
            z = aligned[k]
            tiny = z["gc_size"] <= 0x10 and z["ps2_size"] <= 0x10
            if tiny or ratio_ok(z, GAP_SIZE_TOL):
                accepted[k] = True

    names: Dict[int, Dict[str, Any]] = {}
    report: List[str] = []
    for a, ok in zip(aligned, accepted):
        is_c = a["unit_path"].lower().endswith(".c")
        sig = full_signature(a["signature"], decls)
        try:
            mangled = mangle(sig, is_c)
        except MangleError as e:
            mangled = None
            report.append(f"# mangle error {a['gc']:08X}: {e}")
        status = "ACCEPT" if ok and mangled else "reject"
        report.append(
            f"{status} {a['gc']:08X} gc:{a['gc_size']:5X} ps2:{a['ps2_size']:5X} {a['unit']:22} {mangled or a['name']}"
        )
        if ok and mangled:
            names[a["gc"]] = {"name": mangled, "static": a["static"]}

    # Disambiguate duplicate names (e.g. identical statics in different TUs)
    counts: Dict[str, int] = {}
    for v in names.values():
        counts[v["name"]] = counts.get(v["name"], 0) + 1

    symbols_path = Path("config") / version / "symbols.txt"
    lines = symbols_path.read_text().splitlines()
    existing = {l.split(" = ")[0] for l in lines if " = " in l}
    renamed = 0
    out_lines = []
    for line in lines:
        m = RE_FN_LINE.match(line)
        if m:
            addr = int(m.group(1)[3:], 16)
            v = names.get(addr)
            if v:
                new = v["name"]
                rest = m.group(2)
                if v["static"] or counts[new] > 1:
                    rest = re.sub(r"scope:\w+", "scope:local", rest)
                if counts[new] == 1 and new in existing and not v["static"]:
                    new = None  # already defined elsewhere; keep auto name
                if new:
                    line = f"{new} = {rest}"
                    renamed += 1
        out_lines.append(line)
    symbols_path.write_text("\n".join(out_lines) + "\n")
    report_path.write_text("\n".join(report) + "\n")
    print(f"accepted {sum(accepted)} of {len(aligned)} aligned functions; renamed {renamed} symbols")


if __name__ == "__main__":
    main()
