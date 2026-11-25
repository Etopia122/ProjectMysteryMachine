#!/bin/sh
# Builds the minimal static FFmpeg the PC port links: Bink video and audio
# from files, nothing else (LGPL, no external libraries, no DLLs).
# Needs MSYS2 with mingw-w64-i686-gcc, make and diffutils:
#   pacman -S --needed make diffutils
# usage: sh pc/tools/build_ffmpeg.sh   (then rerun cmake / pc/tools/rebuild.sh)
set -e
VER=8.1.1
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DIR="$ROOT/build/ffmpeg"
mkdir -p "$DIR" && cd "$DIR"

if [ ! -d ffmpeg-$VER ]; then
    [ -f ffmpeg-$VER.tar.xz ] || curl -fsSLO https://ffmpeg.org/releases/ffmpeg-$VER.tar.xz
    [ -f ffmpeg-$VER.tar.xz.asc ] || curl -fsSLO https://ffmpeg.org/releases/ffmpeg-$VER.tar.xz.asc
    # Check the release signature against FFmpeg's release signing key.
    if command -v gpg >/dev/null; then
        export GNUPGHOME="$DIR/gnupg"
        mkdir -p "$GNUPGHOME"
        [ -f ffmpeg-devel.asc ] || curl -fsSLO https://ffmpeg.org/ffmpeg-devel.asc
        gpg -q --import ffmpeg-devel.asc
        gpg --verify ffmpeg-$VER.tar.xz.asc ffmpeg-$VER.tar.xz 2>&1 |
            grep -q "FCF9 86EA 15E6 E293 A564  4F10 B432 2F04 D676 58D8" ||
            { echo "ffmpeg-$VER.tar.xz: bad or unknown signature"; exit 1; }
    fi
    tar xf ffmpeg-$VER.tar.xz
fi

if [ -n "$MINGW_PREFIX" ]; then
    export PATH="$MINGW_PREFIX/bin:/usr/bin:$PATH"
else
    export PATH=/c/msys64/mingw32/bin:/c/msys64/usr/bin:$PATH
fi
win() { cygpath -m "$1" 2>/dev/null || echo "$1"; }
mkdir -p obj && cd obj
../ffmpeg-$VER/configure --prefix="$DIR/install" \
    --arch=x86 --target-os=mingw32 --cc=gcc \
    --enable-static --disable-shared \
    --disable-programs --disable-doc --disable-network --disable-autodetect \
    --disable-everything --disable-avdevice --disable-avfilter \
    --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
    --enable-demuxer=bink --enable-decoder=bink,binkaudio_rdft,binkaudio_dct \
    --enable-protocol=file --disable-x86asm --enable-w32threads >configure.log
# Reproducible objects: the compiler records the path of every file it compiles,
# and FFmpeg puts __FILE__ into its log messages, so without these maps the
# archive would depend on where the build tree sits. configure rewrites
# --extra-cflags and chokes on the "=" in a map, so they go into the generated
# config.mak instead. The executable gets the same treatment in pc/CMakeLists.txt.
printf 'CFLAGS := $(CFLAGS) -ffile-prefix-map=%s=/ffmpeg -ffile-prefix-map=%s=/mingw32\n' \
    "$(win "$DIR/ffmpeg-$VER")" "$(win "${MINGW_PREFIX:-/c/msys64/mingw32}")" >> ffbuild/config.mak
make -j"$(nproc)" >make.log 2>&1
make install >>make.log 2>&1
echo "FFmpeg $VER installed to $DIR/install"
