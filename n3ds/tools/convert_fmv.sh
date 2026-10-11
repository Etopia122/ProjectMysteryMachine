#!/bin/sh
# Re-encodes the game's Bink movies for the New 3DS port: Ogg Theora at 320x240
# (the 4:3 game area on the top screen) with 32 kHz stereo Vorbis (the game's
# mixing rate). The port plays sdmc:/scooby/fmv/<name>.ogv for FMV/<name>.bik
# (FMV/demo/<name>.bik too: the title screen's attract-mode demos).
# usage: sh n3ds/tools/convert_fmv.sh [game files dir] [output dir]
#        (defaults: game/files, build/n3ds/fmv; copy the output to sdmc:/scooby/fmv)
export PATH=/c/msys64/mingw32/bin:$PATH
src=${1:-game/files}
out=${2:-build/n3ds/fmv}
mkdir -p "$out"
for f in "$src"/FMV/*.bik "$src"/FMV/demo/*.bik; do
    name=$(basename "$f" .bik)
    dst="$out/$name.ogv"
    if [ -f "$dst" ] && [ "$dst" -nt "$f" ]; then
        continue
    fi
    echo "$name"
    # Theora needs dimensions in multiples of 16: 320x240 is. Quality 6 of 10
    # keeps decoding light enough for the 3DS CPU.
    ffmpeg -hide_banner -loglevel error -y -i "$f" \
        -vf "scale=320:240:flags=lanczos" -c:v libtheora -q:v 6 \
        -c:a libvorbis -ar 32000 -ac 2 -q:a 3 "$dst" || echo "  failed: $f"
done
du -sh "$out"
