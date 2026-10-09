#!/bin/sh
# Builds with the given game sources at -O0 and screenshots the title at 22 s.
# usage: sh n3ds/tools/o0test.sh '<regex>' <name>
export PATH=/c/msys64/mingw32/bin:$PATH
cmake -S n3ds -B build/n3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/n3ds/cmake/n3ds.cmake "-DN3DS_O0_UNITS=$1" >/dev/null
cmake --build build/n3ds 2>&1 | grep -E " error|FAILED" | head -5
sh n3ds/tools/run.sh build/n3ds/scooby.3dsx 24 22 >/dev/null
powershell -ExecutionPolicy Bypass -File n3ds/tools/cropscale.ps1 build/n3ds/shot_22.png build/n3ds/bis_$2.png 444 53 406 244 1
grep "fps" /c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2/user/sdmc/scooby/log.txt | tail -1
