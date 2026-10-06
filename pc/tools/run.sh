#!/bin/sh
# Run the PC build under gdb for N seconds; prints the log and a backtrace on a crash.
# usage: sh pc/tools/run.sh [seconds] [extra gdb command file]
export PATH=/c/msys64/mingw32/bin:$PATH
cp -u /c/msys64/mingw32/bin/SDL2.dll build/pc/ 2>/dev/null
X=""
[ -n "$2" ] && X="-x $2"
timeout ${1:-30} gdb -q -batch $X -ex run -ex bt -ex "info registers eip" build/pc/scooby.exe > build/pc/run.log 2>&1
grep -v "^\[New Thread\|^\[Thread .* exited\|^\[Inferior" build/pc/run.log
