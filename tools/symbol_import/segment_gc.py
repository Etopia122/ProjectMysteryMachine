#!/usr/bin/env python3

###
# Recovers translation unit (TU) boundaries in .text from data references.
#
# CodeWarrior emits float literals (.sdata2) and string pools (.rodata/.data)
# per TU and never shares them across TUs. Functions referencing the same such
# object are in the same TU, and since TUs are contiguous in .text, so is every
# function between them. This yields a list of "segments": address ranges that
# are known to belong to a single TU (adjacent segments may still be one TU).
#
# Usage:
#   python tools/symbol_import/segment_gc.py build/symbol_import/gc.json out.json
###

import json
import sys
from pathlib import Path


def main() -> None:
    gc = json.loads(Path(sys.argv[1]).read_text())
    out = Path(sys.argv[2])
    funcs = [f for f in gc["functions"] if f["section"] == ".text"]
    objects = gc["objects"]
    strings = gc["strings"]

    # Map each TU-local object to the functions that reference it
    users = {}
    for idx, f in enumerate(funcs):
        for sym, sec in f["refs"].items():
            # .rodata also holds shared const globals (e.g. vector constants),
            # so only its string pools are treated as TU-local
            if sec == ".sdata2" or (sec in (".rodata", ".data") and sym in strings):
                users.setdefault(sym, []).append(idx)

    # Each object spans [first user, last user]; merge overlapping spans
    spans = sorted((min(u), max(u)) for u in users.values())
    merged = []
    for lo, hi in spans:
        if merged and lo <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], hi)
        else:
            merged.append([lo, hi])

    segments = []
    for lo, hi in merged:
        seg_funcs = funcs[lo : hi + 1]
        strs = []
        for f in seg_funcs:
            for sym in f["refs"]:
                if sym in strings:
                    for s in strings[sym][:3]:
                        if s not in strs:
                            strs.append(s)
        segments.append(
            {
                "start": seg_funcs[0]["address"],
                "end": seg_funcs[-1]["address"] + seg_funcs[-1]["size"],
                "first": lo,
                "last": hi,
                "count": hi - lo + 1,
                "strings": strs[:6],
            }
        )

    out.write_text(json.dumps(segments, indent=1))
    prev_last = -1
    for s in segments:
        gap = s["first"] - prev_last - 1
        if gap:
            print(f"   ... {gap} unassigned")
        print(
            f"{s['start']:08X}-{s['end']:08X} {s['count']:4} fns  {' | '.join(s['strings'])[:100]}"
        )
        prev_last = s["last"]
    print(f"{len(segments)} segments")


if __name__ == "__main__":
    main()
