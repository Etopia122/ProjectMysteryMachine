#!/bin/sh
# Side-by-side diff of one function compiled with the game flags.
# Usage: tools/diff_fn.sh <source> <function name substring>
GAMEFLAGS="-O4,s -inline deferred -fp_contract on -char signed -str reuse,pool,readonly -use_lmw_stmw on -lang=c++ -enum int -multibyte -nosyspath"
python tools/compiler_probe/probe.py GIHE78 "$1" --compilers 1.3.2 --variants "$GAMEFLAGS" \
  --extra "-i src/Core/x -i src/Core/gc -i src/Game -i include" --diff "$2"
