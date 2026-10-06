#!/bin/sh
# Run the PC build, attach gdb after N seconds, print the main thread's stack, then stop it.
# usage: sh pc/tools/snap.sh [seconds]
export PATH=/c/msys64/mingw32/bin:$PATH
cp -u /c/msys64/mingw32/bin/SDL2.dll build/pc/ 2>/dev/null
./build/pc/scooby.exe > build/pc/run.log 2>&1 &
PID=$!
sleep ${1:-15}
WPID=$(ps -p $PID -o winpid= 2>/dev/null | tr -d ' ')
[ -z "$WPID" ] && WPID=$(cat /proc/$PID/winpid 2>/dev/null)
gdb -q -batch -p $WPID -ex "thread 1" -ex "bt 25" build/pc/scooby.exe 2>&1 | grep -v "^\[New Thread\|^\[Thread\|warning:" | head -40
kill $PID 2>/dev/null; taskkill //F //PID $WPID >/dev/null 2>&1
grep -v "^\[stub\] _\(GX\|OS\|VI\|PAD\|AI\|AX\|DC\)" build/pc/run.log | head -30
