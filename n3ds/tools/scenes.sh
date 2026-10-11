#!/bin/sh
# Starts New Game in each given scene (PC_SCENE) in Azahar, screenshots it and
# reports the frame rate, unmapped memory accesses (null pointers) and the last log line.
# usage: sh n3ds/tools/scenes.sh F001 G001 ...   (screenshots: build/n3ds/scene_<id>.png)
# N3DS_ENV="KEY=VALUE ..." adds more debug switches; N3DS_TAG names the screenshots scene_<id>_<tag>.png.
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
SD=$AZAHAR/user/sdmc/scooby
# The test switches go in env.txt for each run; the player's own file comes back after.
cp "$SD/env.txt" "$SD/env.txt.scenes-backup" 2>/dev/null
for scene in "$@"; do
    printf 'PC_NOFMV=1\nPC_PROF=1\nPC_SCENE=%s\nPC_AUTOPAD=8:1000,8.3:0,12:100,12.3:0,15:100,15.3:0,18:100,18.3:0,21:100,21.3:0\n' "$scene" > "$SD/env.txt"
    for kv in $N3DS_ENV; do echo "$kv" >> "$SD/env.txt"; done
    rm -f "$SD/log.txt"
    sh n3ds/tools/run.sh build/n3ds/scooby.3dsx 50 48 >/dev/null
    powershell -ExecutionPolicy Bypass -File n3ds/tools/cropscale.ps1 build/n3ds/shot_48.png "build/n3ds/scene_$scene${N3DS_TAG:+_$N3DS_TAG}.png" 444 53 406 244 1
    fps=$(grep "fps," "$SD/log.txt" | tail -1 | sed 's/.*\] //; s/,.*//')
    gpu=$(grep "gpu:" "$SD/log.txt" | tail -1 | sed 's/.*gpu: //; s/, frame begin.*//')
    last=$(grep -v "prof\]" "$SD/log.txt" | tail -1)
    log=$AZAHAR/user/log/azahar_log.txt
    bad=$(grep -c "UnmappedAccess" "$log")
    pcs=$(grep -o "at PC 0x[0-9A-F]*" "$log" | sort | uniq -c | sort -rn | head -3 | awk '{print $4}' | tr '\n' ' ')
    echo "$scene: $fps | $gpu | unmapped $bad $pcs| last: $last"
done
cp "$SD/env.txt.scenes-backup" "$SD/env.txt" 2>/dev/null && rm -f "$SD/env.txt.scenes-backup"
