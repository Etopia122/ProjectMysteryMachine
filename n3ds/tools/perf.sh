#!/bin/sh
# Frame-rate sweep: starts New Game in each scene (PC_SCENE) in Azahar and
# averages the per-second profile over a window of game time (from <from>
# seconds after the app started, <span> seconds long; default 32 and 14), with
# the player standing at the scene's start, or with PERF_WALK=1 walking about
# (changing direction and jumping every two seconds), measured over 34-72 s.
# usage: sh n3ds/tools/perf.sh [-o table.txt] SCENE...   (all scenes: sh n3ds/tools/perf.sh all)
#        sh n3ds/tools/perf.sh [-o table.txt] -q QUEUE   takes scenes one at a time from the
#        file QUEUE (one per line) until it is empty, so several emulator copies can share a sweep.
# Columns: average and lowest fps, ms per frame, then the renderer's share
# (skinning, lighting, drawing with the world sectors in brackets), the time
# spent waiting for the retrace, draws and vertices per frame, and the seconds
# left out as transitions: those with a frame over 300 ms (the game resetting
# the scene after a fall, loading; logged as [long]) or over 31 fps.
# N3DS_ENV="KEY=VALUE ..." adds debug switches; PERF_FROM / PERF_SPAN change the window.
# The app (PERF_APP, default build/n3ds/scooby.3dsx) is copied first, so rebuilding
# during a sweep doesn't change it; AZAHAR / AZAHAR_EXE pick the emulator copy.
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
EXE=${AZAHAR_EXE:-azahar.exe}
SD=$AZAHAR/user/sdmc/scooby
APP=build/n3ds/perf_$$.3dsx
cp "${PERF_APP:-build/n3ds/scooby.3dsx}" "$APP"
PAD=8:1000,8.3:0,12:100,12.3:0,15:100,15.3:0,18:100,18.3:0,21:100,21.3:0
if [ -n "$PERF_WALK" ]; then
    FROM=${PERF_FROM:-34}
    SPAN=${PERF_SPAN:-38}
    # Directions to walk in turn (stick x y), with a jump at each change.
    set_dirs="0:100 70:70 0:100 -70:70 100:0 0:100 -100:0 0:-100 70:-70 0:100"
    t=26
    for k in 1 2; do
        for d in $set_dirs; do
            PAD="$PAD,$t:100:$d,$t.3:0:$d"
            t=$((t + 2))
        done
    done
    PAD="$PAD,$t:0"
else
    FROM=${PERF_FROM:-32}
    SPAN=${PERF_SPAN:-14}
fi
END=$((FROM + SPAN))
out=/dev/stdout
if [ "$1" = "-o" ]; then
    out=$2
    shift 2
fi
queue=
if [ "$1" = "-q" ]; then
    queue=$2
    shift 2
fi
# The next scene of the queue (mkdir is the lock between sweeps).
next_scene() {
    until mkdir "$queue.lock" 2>/dev/null; do sleep 1; done
    scene=$(head -1 "$queue")
    sed -i 1d "$queue"
    rmdir "$queue.lock"
    [ -n "$scene" ]
}
if [ "$1" = "all" ]; then
    set -- $(for d in game/files/*/; do ls "$d"; done | grep -i '\.hip$' | grep -iv '^mnu' | sed 's/\.[Hh][Ii][Pp]$//' | tr a-z A-Z)
fi
cp "$SD/env.txt" "$SD/env.txt.perf-backup" 2>/dev/null
printf '%-5s %5s %5s %6s %5s %5s %5s %6s %5s %5s %6s %7s\n' scene fps min ms skin light draw world wait draws verts stalls >> "$out"
if [ -n "$queue" ]; then
    set -- queue
fi
for scene in "$@"; do
  while :; do
    if [ -n "$queue" ]; then
        next_scene || break
    fi
    printf 'PC_NOFMV=1\nPC_SAVE_DIR=testsave\nPC_PROF=1\nPC_SCENE=%s\nPC_AUTOPAD=%s\n' "$scene" "$PAD" > "$SD/env.txt"
    for kv in $N3DS_ENV; do echo "$kv" >> "$SD/env.txt"; done
    rm -f "$SD/log.txt"
    # Run until the game's own clock passes the window (the emulator may run
    # slower than real time), at most three minutes.
    taskkill //IM $EXE //F >/dev/null 2>&1
    ("$AZAHAR/$EXE" "$(cygpath -w "$APP")" >/dev/null 2>&1 &)
    start=$(date +%s)
    while :; do
        sleep 2
        last=$(tail -1 "$SD/log.txt" 2>/dev/null | awk '{print int($1)}')
        [ -n "$last" ] && [ "$last" -gt "$END" ] && break
        [ $(($(date +%s) - start)) -gt 180 ] && break
    done
    taskkill //IM $EXE //F >/dev/null 2>&1
    awk -v from="$FROM" -v end="$END" -v scene="$scene" '
        { t = $1 + 0 }
        t < from || t > end { next }
        /\[long\] frame/ { if ($4 + 0 > 300) lastlong = t; next }
        / fps, / && (t - lastlong < 1.1 || $3 + 0 > 31) { skipped++; next }
        / fps, / { f = $3 + 0; fs += f; n++; if (min == 0 || f < min) min = f
                   for (i = 1; i <= NF; i++) {
                       if ($i == "ms/frame:") ms += $(i - 1)
                       if ($i == "skin") sk += $(i + 1)
                       if ($i == "light") li += $(i + 1)
                       if ($i == "draw") dr += $(i + 1)
                       if ($i == "(world") wo += $(i + 1) } }
        /VIWaitForRetrace/ { for (i = 1; i <= NF; i++) if ($i == "waiting") { w += $(i - 2); nw++ } }
        /gpu: / { d += $4; v += $6; ng++ }
        END {
            if (n == 0) { printf "%-5s  no profile (did not load?)\n", scene; exit }
            printf "%-5s %5.1f %5.1f %6.1f %5.1f %5.1f %5.1f %6.1f %5.1f %5d %6d %7d\n", scene, fs / n, min, ms / n, sk / n,
                   li / n, dr / n, wo / n, nw ? w / nw / (fs / n) : 0, ng ? d / ng : 0, ng ? v / ng : 0, skipped }' "$SD/log.txt" >> "$out"
    if [ -n "$PERF_KEEPLOGS" ]; then
        mkdir -p build/n3ds/perf_logs
        cp "$SD/log.txt" "build/n3ds/perf_logs/$scene.txt"
    fi
    [ -n "$queue" ] || break
  done
done
cp "$SD/env.txt.perf-backup" "$SD/env.txt" 2>/dev/null
rm -f "$APP"
