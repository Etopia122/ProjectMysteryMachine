#!/bin/sh
# Rebuild the PC executable: game library, regenerate stubs, link.
# usage: sh pc/tools/rebuild.sh
set -e
export PATH=/c/msys64/mingw32/bin:$PATH
cmake -S pc -B build/pc -G Ninja -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build build/pc --target scoobygame | grep -E "error|warning: undefined" || true
cmake --build build/pc --target scooby >/dev/null 2>&1 || true   # compile PC sources for genstubs
python pc/tools/genstubs.py
cmake -S pc -B build/pc >/dev/null
cmake --build build/pc | grep -E "error|undefined|multiple|FAILED" || true
