#!/bin/sh
# Runs the 3DS build in Azahar with test switches and screenshots the window at
# moments of game time (the log's clock, which can run slower than real time),
# then puts the player's env.txt back. Saves go to sdmc:/scooby/testsave.
# usage: N3DS_ENV="KEY=VALUE ..." sh n3ds/tools/shots.sh <app> <time>...
#        screenshots: build/n3ds/shot_<time>.png (both screens: crop with cropscale.ps1)
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
EXE=${AZAHAR_EXE:-azahar.exe}
SD=$AZAHAR/user/sdmc/scooby
app=$1
shift
cp "$SD/env.txt" "$SD/env.txt.shots-backup"
printf 'PC_PROF=1\nPC_SAVE_DIR=testsave\n' > "$SD/env.txt"
for kv in $N3DS_ENV; do echo "$kv" >> "$SD/env.txt"; done
rm -f "$SD/log.txt"
taskkill //IM $EXE //F >/dev/null 2>&1
("$AZAHAR/$EXE" "$(cygpath -w "$app")" >/dev/null 2>&1 &)
start=$(date +%s)
for at in "$@"; do
    while :; do
        sleep 1
        last=$(tail -1 "$SD/log.txt" 2>/dev/null | awk '{print int($1)}')
        [ -n "$last" ] && [ "$last" -ge "${at%.*}" ] && break
        [ $(($(date +%s) - start)) -gt 240 ] && break
    done
    powershell -ExecutionPolicy Bypass -File n3ds/tools/winshot.ps1 ${EXE%.exe} "$(cygpath -w build/n3ds)\\shot_$at.png" >/dev/null
done
taskkill //IM $EXE //F >/dev/null 2>&1
cp "$SD/env.txt.shots-backup" "$SD/env.txt" && rm -f "$SD/env.txt.shots-backup"
