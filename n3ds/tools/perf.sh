#!/bin/sh
# Frame-rate sweep: starts New Game in each scene (PC_SCENE) in Azahar and
# averages the per-second profile from <from> seconds on (default 32), with the
# player standing at the scene's start.
# usage: sh n3ds/tools/perf.sh [-o table.txt] SCENE...   (all scenes: sh n3ds/tools/perf.sh all)
# Columns: average and lowest fps, ms per frame, then the renderer's share
# (skinning, lighting, drawing with the world sectors in brackets), the time
# spent waiting for the retrace, draws and vertices per frame.
# N3DS_ENV="KEY=VALUE ..." adds debug switches; PERF_FROM / PERF_SECS change the window.
# The app (PERF_APP, default build/n3ds/scooby.3dsx) is copied first, so rebuilding
# during a sweep doesn't change it; AZAHAR / AZAHAR_EXE pick the emulator copy.
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
SD=$AZAHAR/user/sdmc/scooby
APP=build/n3ds/perf_$$.3dsx
cp "${PERF_APP:-build/n3ds/scooby.3dsx}" "$APP"
FROM=${PERF_FROM:-32}
SECS=${PERF_SECS:-46}
out=/dev/stdout
if [ "$1" = "-o" ]; then
    out=$2
    shift 2
fi
if [ "$1" = "all" ]; then
    set -- $(for d in game/files/*/; do ls "$d"; done | grep -i '\.hip$' | grep -iv '^mnu' | sed 's/\.[Hh][Ii][Pp]$//' | tr a-z A-Z)
fi
cp "$SD/env.txt" "$SD/env.txt.perf-backup" 2>/dev/null
printf '%-5s %5s %5s %6s %5s %5s %5s %6s %5s %5s %6s\n' scene fps min ms skin light draw world wait draws verts >> "$out"
for scene in "$@"; do
    printf 'PC_NOFMV=1\nPC_PROF=1\nPC_SCENE=%s\nPC_AUTOPAD=8:1000,8.3:0,12:100,12.3:0,15:100,15.3:0,18:100,18.3:0,21:100,21.3:0\n' "$scene" > "$SD/env.txt"
    for kv in $N3DS_ENV; do echo "$kv" >> "$SD/env.txt"; done
    rm -f "$SD/log.txt"
    sh n3ds/tools/run.sh "$APP" "$SECS" >/dev/null 2>&1
    awk -v from="$FROM" -v scene="$scene" '
        { t = $1 + 0 }
        t < from { next }
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
            printf "%-5s %5.1f %5.1f %6.1f %5.1f %5.1f %5.1f %6.1f %5.1f %5d %6d\n", scene, fs / n, min, ms / n, sk / n,
                   li / n, dr / n, wo / n, nw ? w / nw / (fs / n) : 0, ng ? d / ng : 0, ng ? v / ng : 0 }' "$SD/log.txt" >> "$out"
done
cp "$SD/env.txt.perf-backup" "$SD/env.txt" 2>/dev/null
rm -f "$APP"
