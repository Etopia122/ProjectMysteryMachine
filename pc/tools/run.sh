#!/bin/sh
# Run the PC build for a few seconds under gdb; prints stubs hit and a backtrace on crash.
# usage: sh pc/tools/run.sh [seconds]
export PATH=/c/msys64/mingw32/bin:$PATH
cp -u /c/msys64/mingw32/bin/SDL2.dll build/pc/ 2>/dev/null
timeout ${1:-30} gdb -q -batch -ex run -ex bt -ex "info registers eip" build/pc/scooby.exe > build/pc/run.log 2>&1
grep -v "^\[New Thread\|^\[Thread .* exited\|^\[Inferior" build/pc/run.log
