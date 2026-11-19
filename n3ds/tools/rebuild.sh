#!/bin/sh
# Rebuild the New 3DS app: game library, regenerate stubs, link, pack .3dsx.
# usage: sh n3ds/tools/rebuild.sh
set -e
if [ -z "$MINGW_PREFIX" ] && [ -d /c/msys64/mingw32/bin ]; then
    export PATH=/c/msys64/mingw32/bin:$PATH
fi
B=build/n3ds
cfg() { cmake -S n3ds -B $B -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/n3ds/cmake/n3ds.cmake >/dev/null; }
[ -f n3ds/src/stubs_gen.S ] || echo "@ placeholder" > n3ds/src/stubs_gen.S
cfg
cmake --build $B --target scoobygame | grep -E "error|FAILED" || true
cmake --build $B --target scooby >/dev/null 2>&1 || true   # compile the platform sources for genstubs
python n3ds/tools/genstubs.py $B
cfg
cmake --build $B | grep -E "error|undefined|multiple|FAILED" || true
ls -la $B/scooby.3dsx
