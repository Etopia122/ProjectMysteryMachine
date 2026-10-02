#!/bin/sh
# Side-by-side diff of one function, compiled with the exact flags the build
# uses for that source file (taken from objdiff.json).
# Usage: tools/diff_fn.sh <source> <function name substring>
SRC="$1"
FLAGS=$(python - "$SRC" <<'PY'
import json, sys
from pathlib import Path
src = Path(sys.argv[1]).as_posix()
for u in json.load(open("objdiff.json"))["units"]:
    meta = u.get("metadata", {})
    if meta.get("source_path") == src:
        print(u["scratch"]["c_flags"])
        break
else:
    # Not a configured unit (e.g. a scratch file): use the game flags of xBase
    for u in json.load(open("objdiff.json"))["units"]:
        if u.get("metadata", {}).get("source_path") == "src/Core/x/xBase.cpp":
            print(u["scratch"]["c_flags"])
PY
)
COMPILER=$(python -c "import json;u=[x for x in json.load(open('objdiff.json'))['units'] if x.get('metadata',{}).get('source_path')=='src/Core/x/xBase.cpp'][0];print(u['scratch']['compiler'])")
python tools/compiler_probe/probe.py GIHE78 "$SRC" --compilers 1.3.2 --variants "$FLAGS" \
  --extra "-i src/Core/x -i src/Core/gc -i src/Game -i include" --diff "$2"
