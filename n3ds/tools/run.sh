#!/bin/sh
# Runs a .3dsx in the Azahar emulator for a while, screenshots the window at
# the given times, then closes it and prints the app's debug output.
# usage: sh n3ds/tools/run.sh <file.3dsx> <seconds> [shot_time ...]
# Screenshots go to build/n3ds/shot_<time>.png. AZAHAR overrides the emulator path,
# AZAHAR_EXE its executable name (a renamed copy can run beside another).
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
EXE=${AZAHAR_EXE:-azahar.exe}
app=$(cygpath -w "$1" 2>/dev/null || echo "$1")
secs=$2
shift 2
mkdir -p build/n3ds
taskkill //IM $EXE //F >/dev/null 2>&1
rm -f "$AZAHAR/user/log/azahar_log.txt"
("$AZAHAR/$EXE" "$app" >/dev/null 2>&1 &)
t=0
for at in "$@"; do
    sleep $((at - t))
    t=$at
    powershell -ExecutionPolicy Bypass -File n3ds/tools/winshot.ps1 ${EXE%.exe} "$(cygpath -w build/n3ds)\\shot_$at.png" >/dev/null
done
[ "$secs" -gt "$t" ] && sleep $((secs - t))
taskkill //IM $EXE //F >/dev/null 2>&1
grep -E "Debug.Emulated|Critical|HW.Memory <Error>|Unmapped" "$AZAHAR/user/log/azahar_log.txt" | sed 's/.*OutputDebugString:[0-9]*: //' | head -100
