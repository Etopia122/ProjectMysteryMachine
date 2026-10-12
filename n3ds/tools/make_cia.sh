#!/bin/sh
# Builds the New 3DS port's CIA (and a .cxi that emulators can boot directly).
#   sh n3ds/tools/make_cia.sh            game files + converted movies inside (personal use only:
#                                        a CIA with the game's data must not be shared)
#   sh n3ds/tools/make_cia.sh --no-fmv   game files only (faster to build and install, for
#                                        testing): build/n3ds/scooby_nofmv.cia/.cxi, movies from
#                                        sdmc:/scooby/fmv if they are there
#   sh n3ds/tools/make_cia.sh --no-data  the app alone; files and movies stay in sdmc:/scooby
# Needs makerom and bannertool in $CTR_TOOLS (default C:/Users/Ethan/Tools/3ds), a built
# build/n3ds/scooby (sh n3ds/tools/rebuild.sh) and, for the movies, convert_fmv.sh's output.
# Output: build/n3ds/scooby.cia and build/n3ds/scooby.cxi (scooby_nofmv.* with --no-fmv)
set -e
TOOLS=${CTR_TOOLS:-/c/Users/Ethan/Tools/3ds}
MAKEROM=$TOOLS/makerom.exe
BANNERTOOL=$(ls "$TOOLS"/bannertool*/bannertool.exe "$TOOLS"/bannertool.exe 2>/dev/null | head -1)
FILES=${SCOOBY_FILES:-game/files}
FMV=${SCOOBY_FMV:-build/n3ds/fmv}
B=build/n3ds
ROMFS=$B/romfs
OUT=scooby
[ "$1" = "--no-fmv" ] && OUT=scooby_nofmv

[ -x "$MAKEROM" ] || { echo "makerom not found in $TOOLS"; exit 1; }
[ -n "$BANNERTOOL" ] || { echo "bannertool not found in $TOOLS"; exit 1; }
[ -f $B/scooby ] || { echo "build the app first: sh n3ds/tools/rebuild.sh"; exit 1; }

# RomFS: junctions to the game's folders (not the Bink movies, which the 3DS
# doesn't use), hard links to its root files, and the converted movies.
win() { cygpath -w "$1"; }
rm -rf "$ROMFS"
mkdir -p "$ROMFS"
if [ "$1" != "--no-data" ]; then
    [ -f "$FILES/boot.HIP" ] || { echo "no game files in $FILES"; exit 1; }
    mkdir -p "$ROMFS/files"
    for d in "$FILES"/*/; do
        name=$(basename "$d")
        case "$(echo "$name" | tr 'A-Z' 'a-z')" in fmv) continue;; esac
        cmd //c "mklink /J $(win "$ROMFS/files/$name") $(win "$d")" >/dev/null
    done
    for f in "$FILES"/*; do
        [ -f "$f" ] || continue
        cmd //c "mklink /H $(win "$ROMFS/files/$(basename "$f")") $(win "$f")" >/dev/null
    done
    if [ "$1" != "--no-fmv" ]; then
        # Converts any movie not converted yet (all of them the first time).
        sh n3ds/tools/convert_fmv.sh "$FILES" "$FMV"
        cmd //c "mklink /J $(win "$ROMFS/fmv") $(win "$FMV")" >/dev/null
    fi
fi

# The bottom screen's background (n3ds/tools/make_bottom_bg.py), from the
# player's picture at game/n3ds_bottom_bg.png (BOTTOM_BG overrides) if there is one.
BG=${BOTTOM_BG:-game/n3ds_bottom_bg.png}
if [ -f "$BG" ]; then
    python n3ds/tools/make_bottom_bg.py "$BG" "$ROMFS/bottom_bg.rgba" >/dev/null
fi

# HOME Menu icon and banner, from the disc's GameCube memory card icon/banner.
python n3ds/tools/make_cia_art.py "$FILES" $B/cia_art >/dev/null
"$BANNERTOOL" makebanner -i $B/cia_art/banner.png -a $B/cia_art/banner.wav -o $B/cia_art/banner.bnr >/dev/null
"$BANNERTOOL" makesmdh -s "Scooby-Doo! Night of 100 Frights" -l "Experimental New 3DS port" \
    -p "Scooby-Doo decomp" -i $B/cia_art/icon.png -o $B/cia_art/icon.icn >/dev/null

for fmt in cxi cia; do
    "$MAKEROM" -f $fmt -o $B/$OUT.$fmt -target t -exefslogo -elf $B/scooby -rsf n3ds/cia.rsf \
        -icon $B/cia_art/icon.icn -banner $B/cia_art/banner.bnr -DAPP_ROMFS="$ROMFS"
done
ls -la $B/$OUT.cia $B/$OUT.cxi
