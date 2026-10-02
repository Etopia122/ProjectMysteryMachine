#!/usr/bin/env python3

###
# Derives splits.txt entries for TUs that also exist in a reference decomp
# (dtk-style symbols.txt + splits.txt), by matching symbol names.
#
# For each section of a reference TU, every reference symbol in that section
# whose name also exists in our symbols.txt (same section) votes for where the
# section starts in our binary. A section is emitted only if all votes agree and
# every one of our symbols inside the resulting range has a counterpart at the
# same offset in the reference (or is auto-named).
#
# Usage:
#   python tools/import_splits.py <version> <ref symbols.txt> <ref splits.txt> <ref TU> [<ref TU> ...]
#
# Fallback when no names match: the data objects our (already placed) .text
# references in that section are aligned against the reference's symbol
# offsets; the single shift that puts every referenced object on a reference
# symbol start is used. Requires build/symbol_import/gc.json (analyze_gc.py).
###

import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Dict, List, Optional, Tuple

RE_SYMBOL = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // type:(\w+)(?: size:0x([0-9A-Fa-f]+))?")
RE_UNIT = re.compile(r"^(\S.*):\s*(.*)$")
RE_SECTION = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)(.*)$")
AUTO = ("fn_", "lbl_", "gap_", "jumptable_", "@", "...", "$")


def load_symbols(path: Path) -> List[Tuple[str, str, int, int]]:
    out = []
    for line in path.read_text().splitlines():
        m = RE_SYMBOL.match(line)
        if m:
            out.append((m.group(1), m.group(2), int(m.group(3), 16), int(m.group(5), 16) if m.group(5) else 0))
    return out


def load_splits(path: Path) -> Dict[str, List[Tuple[str, int, int, str]]]:
    units: Dict[str, List[Tuple[str, int, int, str]]] = {}
    cur: Optional[str] = None
    for line in path.read_text().splitlines():
        if line.startswith("Sections:"):
            cur = None
            continue
        m = RE_UNIT.match(line)
        if m and not line.startswith(("\t", " ")):
            cur = m.group(1)
            units[cur] = []
            continue
        m = RE_SECTION.match(line)
        if m and cur:
            units[cur].append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4).strip()))
    return units


def base_name(name: str) -> str:
    # dtk disambiguates local symbols as name_ADDR; compare without the suffix
    return re.sub(r"_[0-9A-F]{8}$", "", name)


_GC = None


def ref_shift_from_refs(text_range, sec, start, end, refs):
    """Finds our section start from the objects our .text references."""
    global _GC
    if _GC is None:
        _GC = json.loads(Path("build/symbol_import/gc.json").read_text())
    objs = _GC["objects"]
    addrs = set()
    for f in _GC["functions"]:
        if text_range[0] <= f["address"] < text_range[1]:
            for sym, osec in f["refs"].items():
                if osec == sec and sym in objs:
                    addrs.add(objs[sym]["address"])
    if not addrs:
        return None
    ref_offsets = {a - start for _, s, a, _ in refs if s == sec and start <= a < end}
    shifts = None
    for a in addrs:
        cand = {a - off for off in ref_offsets}
        shifts = cand if shifts is None else shifts & cand
    if shifts and len(shifts) == 1:
        return next(iter(shifts))
    return None


def main() -> None:
    args = sys.argv[1:]
    version, ref_symbols, ref_splits = args[0], Path(args[1]), Path(args[2])
    tus = args[3:]
    ours = load_symbols(Path("config") / version / "symbols.txt")
    refs = load_symbols(ref_symbols)
    ref_units = load_splits(ref_splits)

    our_by_name: Dict[Tuple[str, str], List[Tuple[int, int]]] = {}
    for name, sec, addr, size in ours:
        our_by_name.setdefault((base_name(name), sec), []).append((addr, size))

    for tu in tus:
        if tu not in ref_units:
            print(f"# {tu}: not in reference splits", file=sys.stderr)
            continue
        lines = []
        ok = True
        text_range = None
        for sec, start, end, attrs in ref_units[tu]:
            votes = Counter()
            for name, rsec, raddr, rsize in refs:
                if rsec != sec or not (start <= raddr < end) or name.startswith(AUTO):
                    continue
                cands = our_by_name.get((base_name(name), sec), [])
                if len(cands) == 1:
                    votes[cands[0][0] - (raddr - start)] += 1
            if not votes and text_range:
                shift = ref_shift_from_refs(text_range, sec, start, end, refs)
                if shift is not None:
                    votes[shift] += 1
            if len(votes) != 1:
                print(f"# {tu} {sec}: {'no votes' if not votes else 'conflicting votes ' + str(dict(votes))}", file=sys.stderr)
                ok = False
                break
            our_start = next(iter(votes))
            if sec == ".text":
                text_range = (our_start, our_start + (end - start))
            our_end = our_start + (end - start)
            # Every named symbol of ours in range must sit at the same offset in the reference
            ref_offsets = {base_name(n): a - start for n, s, a, _ in refs if s == sec and start <= a < end}
            for name, osec, oaddr, _ in ours:
                if osec == sec and our_start <= oaddr < our_end and not name.startswith(AUTO):
                    if ref_offsets.get(base_name(name)) != oaddr - our_start:
                        print(f"# {tu} {sec}: {name} @ {oaddr:08X} not at matching offset", file=sys.stderr)
                        ok = False
            lines.append(f"\t{sec:<11} start:0x{our_start:08X} end:0x{our_end:08X}{(' ' + attrs) if attrs else ''}")
        if ok:
            print(f"\n{tu}:")
            print("\n".join(lines))


if __name__ == "__main__":
    main()
