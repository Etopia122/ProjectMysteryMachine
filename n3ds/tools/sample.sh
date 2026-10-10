#!/bin/sh
# Poor man's sampling profiler for the 3DS build in Azahar: after <warmup>
# seconds the app's sample thread (N3DS_SAMPLE, added to env.txt for the run)
# hits n3ds_sample_point four times a second; GDB breaks there through Azahar's
# stub and prints the main thread's backtrace, <count> times. The game's other
# switches come from that Azahar's env.txt; use_gdbstub is on for the run only.
# usage: sh n3ds/tools/sample.sh <warmup> <count>   (output: build/n3ds/samples.txt)
# AZAHAR / AZAHAR_EXE pick another Azahar copy (e.g. one renamed so a sweep can run alongside).
# Summaries: the innermost functions, then every function's share of samples.
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
EXE=${AZAHAR_EXE:-azahar.exe}
GDB=/c/msys64/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb.exe
warmup=$1
count=$2
SD=$AZAHAR/user/sdmc/scooby
cp "$SD/env.txt" "$SD/env.txt.sample-backup"
echo "N3DS_SAMPLE=$warmup" >> "$SD/env.txt"
cfg=$AZAHAR/user/config/qt-config.ini
sed -i 's/^use_gdbstub=false/use_gdbstub=true/; s/^use_gdbstub.default=true/use_gdbstub\\default=false/' "$cfg"
taskkill //IM $EXE //F >/dev/null 2>&1
("$AZAHAR/$EXE" "$(cygpath -w build/n3ds/scooby.3dsx)" >/dev/null 2>&1 &)
sleep 3
{
    echo "set pagination off"
    echo "set confirm off"
    echo "target remote localhost:24689"
    echo "break n3ds_sample_point"
    echo "continue"
    i=0
    while [ $i -lt "$count" ]; do
        echo "thread apply 1 bt 30"
        echo "continue"
        i=$((i + 1))
    done
    echo "detach"
} > build/n3ds/sample.gdb
timeout $((warmup + count + 300)) "$GDB" -batch -x build/n3ds/sample.gdb build/n3ds/scooby > build/n3ds/samples.txt 2>&1
taskkill //IM $EXE //F >/dev/null 2>&1
sed -i 's/use_gdbstub=true/use_gdbstub=false/' "$cfg"
cp "$SD/env.txt.sample-backup" "$SD/env.txt"
fn() { awk '{for (i = 1; i <= NF; i++) if ($i == "in") { print $(i + 1); next } print $2}'; }
n=$(grep -c '^#0 ' build/n3ds/samples.txt)
echo "samples: $n"
echo "-- innermost"
grep "^#0 " build/n3ds/samples.txt | fn | sort | uniq -c | sort -rn | head -25
echo "-- inclusive (samples with the function anywhere in the stack)"
awk '/^Thread 1 /{s++} /^#[0-9]+ /{f=$2; for (i = 1; i <= NF; i++) if ($i == "in") { f = $(i + 1); break }
     if (!seen[s, f]++) c[f]++} END {for (f in c) print c[f], f}' build/n3ds/samples.txt | sort -rn | head -40
