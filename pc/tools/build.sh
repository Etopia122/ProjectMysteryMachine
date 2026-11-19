#!/bin/sh
# Build only the PC port (the New 3DS build is separate: n3ds/tools/rebuild.sh).
# Produces build/pc/scooby.exe (release, no console window), build/pc/scooby_debug.exe
# (console, for testing; never packaged) and build/pc/scooby_launcher.exe.
# usage: sh pc/tools/build.sh
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
if [ -z "$MINGW_PREFIX" ] && [ -d /c/msys64/mingw32/bin ]; then
    export PATH=/c/msys64/mingw32/bin:$PATH
fi
BUILD=${PC_BUILD_DIR:-build/pc}
PYTHON=${PYTHON:-python}

cmake -S pc -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --target scoobygame
# Compile the PC sources once so genstubs can see which symbols they define.
# The link may fail here (the old stub file is still in place); the objects are
# what matter, and the final build below links with the regenerated stubs.
cmake --build "$BUILD" --target scooby || true
"$PYTHON" pc/tools/genstubs.py "$BUILD"
cmake --build "$BUILD"
