#!/bin/sh
# Poor man's sampling profiler for the 3DS build in Azahar: a breakpoint in the
# VI retrace thread (every 16.7 ms) prints the main thread's backtrace, after
# <skip> retraces of warm-up, for <count> samples. Needs use_gdbstub=true.
# usage: sh n3ds/tools/sample.sh <skip> <count>   (output: build/n3ds/samples.txt)
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
GDB=/c/msys64/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb.exe
skip=$1
count=$2
cfg=$AZAHAR/user/config/qt-config.ini
sed -i 's/^use_gdbstub=false/use_gdbstub=true/; s/^use_gdbstub.default=true/use_gdbstub\default=false/' "$cfg"
taskkill //IM azahar.exe //F >/dev/null 2>&1
("$AZAHAR/azahar.exe" "$(cygpath -w build/n3ds/scooby.3dsx)" >/dev/null 2>&1 &)
sleep 8
{
    echo "set pagination off"
    echo "set confirm off"
    echo "target remote localhost:24689"
    echo "break vi_time.cpp:36"
    echo "ignore 1 $skip"
    echo "continue"
    i=0
    while [ $i -lt "$count" ]; do
        echo "thread apply 1 bt 12"
        echo "continue"
        i=$((i + 1))
    done
    echo "detach"
} > build/n3ds/sample.gdb
timeout 600 "$GDB" -batch -x build/n3ds/sample.gdb build/n3ds/scooby > build/n3ds/samples.txt 2>&1
taskkill //IM azahar.exe //F >/dev/null 2>&1
sed -i 's/use_gdbstub=true/use_gdbstub=false/' "$cfg"
# Summary: the innermost game/platform function of each sample.
grep -E "^#[0-9]+ " build/n3ds/samples.txt | awk '{for (i = 1; i <= NF; i++) if ($i == "in") { print $(i + 1); next } print $2}' > build/n3ds/frames.txt
echo "samples: $(grep -c '^#0 ' build/n3ds/samples.txt)"
grep -A0 "^#0 " build/n3ds/samples.txt | awk '{for (i = 1; i <= NF; i++) if ($i == "in") { print $(i + 1); next } print $2}' | sort | uniq -c | sort -rn | head -25
