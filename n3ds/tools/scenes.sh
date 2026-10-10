#!/bin/sh
# Starts New Game in each given scene (PC_SCENE) in Azahar, screenshots it and
# reports the frame rate and any errors from the log.
# usage: sh n3ds/tools/scenes.sh F001 G001 ...   (screenshots: build/n3ds/scene_<id>.png)
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
SD=$AZAHAR/user/sdmc/scooby
for scene in "$@"; do
    printf 'PC_NOFMV=1\nPC_PROF=1\nPC_SCENE=%s\nPC_AUTOPAD=8:1000,8.3:0,12:100,12.3:0,15:100,15.3:0,18:100,18.3:0,21:100,21.3:0\n' "$scene" > "$SD/env.txt"
    rm -f "$SD/log.txt"
    sh n3ds/tools/run.sh build/n3ds/scooby.3dsx 50 48 >/dev/null
    powershell -ExecutionPolicy Bypass -File n3ds/tools/cropscale.ps1 build/n3ds/shot_48.png "build/n3ds/scene_$scene.png" 444 53 406 244 1
    fps=$(grep "fps," "$SD/log.txt" | tail -1 | sed 's/.*\] //; s/,.*//')
    gpu=$(grep "gpu:" "$SD/log.txt" | tail -1 | sed 's/.*gpu: //; s/, frame begin.*//')
    last=$(grep -v "prof\]" "$SD/log.txt" | tail -1)
    echo "$scene: $fps | $gpu | last: $last"
done
