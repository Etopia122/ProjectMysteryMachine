#!/usr/bin/env python3
# Build the game from the decompiled sources (--non-matching, linking every
# unit marked Equivalent) and write the resulting main.dol into a copy of the
# original disc image, for testing in Dolphin or on hardware.
#
# usage: python tools/make_test_iso.py [--no-build] [--autoplay script.txt] [out.iso]
#
# --autoplay compiles scripted controller input into iPad.cpp (test builds
# only) and writes build/autoplay/Scooby-AUTOPLAY-do-not-play.iso instead, kept
# out of build/test so it is never mistaken for the playable image.
#   <frame> <frames> <BUTTONS joined by +> [stickX stickY]   (# comments)
# e.g. "600 5 START" or "900 120 - 0 80" (stick up, no buttons).
#
# The default (matching) build configuration is restored afterwards.
import os
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ISO_IN = os.path.join(ROOT, "orig", "GIHE78", "Scooby.iso")
DOL_OUT = os.path.join(ROOT, "build", "GIHE78", "main.dol")
TEST_DIR = os.path.join(ROOT, "build", "test")


def run(*cmd):
    print("+", " ".join(cmd))
    subprocess.check_call(cmd, cwd=ROOT)


BUTTONS = {"LEFT": 0x1, "RIGHT": 0x2, "DOWN": 0x4, "UP": 0x8, "Z": 0x10, "R": 0x20, "L": 0x40,
           "A": 0x100, "B": 0x200, "X": 0x400, "Y": 0x800, "START": 0x1000, "-": 0}


HEADER = """struct ScoobAutoplayEvent
{
    unsigned int start;
    unsigned int len;
    unsigned short buttons;
    signed char stickX;
    signed char stickY;
};

static const ScoobAutoplayEvent sAutoplay[] = {
"""


def write_autoplay(script):
    rows = []
    for line in open(script):
        line = line.split("#")[0].split()
        if not line:
            continue
        start, length, btns = int(line[0]), int(line[1]), line[2]
        mask = 0
        for b in btns.split("+"):
            mask |= BUTTONS[b.upper()]
        sx = int(line[3]) if len(line) > 3 else 0
        sy = int(line[4]) if len(line) > 4 else 0
        rows.append("    { %d, %d, 0x%X, %d, %d }," % (start, length, mask, sx, sy))
    with open(os.path.join(TEST_DIR, "autoplay.h"), "w") as f:
        f.write(HEADER + "\n".join(rows) + "\n};\n")


def build_source_dol(dst):
    run(sys.executable, "configure.py", "--non-matching")
    try:
        run("ninja")
        shutil.copyfile(DOL_OUT, dst)
        shutil.copyfile(DOL_OUT.replace("main.dol", "main.elf"), dst.replace("main.dol", "main.elf"))
    finally:
        run(sys.executable, "configure.py")


def patch_iso(iso, dol):
    data = open(dol, "rb").read()
    with open(iso, "r+b") as f:
        f.seek(0x420)
        dol_off, fst_off = struct.unpack(">II", f.read(8))
        room = fst_off - dol_off
        if len(data) <= room and dol_off < fst_off:
            f.seek(dol_off)
            f.write(data)
            f.write(b"\0" * (room - len(data)))
        else:
            # Doesn't fit before the FST: put it in the free space after the
            # last file (the image is a full-size disc) and repoint the header.
            new_off = 0x50000000
            f.seek(new_off)
            f.write(data)
            f.seek(0x420)
            f.write(struct.pack(">I", new_off))
            dol_off, room = new_off, len(data)
    print("wrote 0x%X-byte main.dol at 0x%X" % (len(data), dol_off))


def main():
    argv = sys.argv[1:]
    autoplay = None
    if "--autoplay" in argv:
        i = argv.index("--autoplay")
        autoplay = argv[i + 1]
        del argv[i:i + 2]
    args = [a for a in argv if not a.startswith("--")]
    os.makedirs(TEST_DIR, exist_ok=True)
    dol = os.path.join(TEST_DIR, "main.dol")
    if autoplay:
        write_autoplay(autoplay)
        os.environ["SCOOB_AUTOPLAY"] = "1"
        if not args:
            args = [os.path.join(ROOT, "build", "autoplay", "Scooby-AUTOPLAY-do-not-play.iso")]
    if "--no-build" not in sys.argv:
        build_source_dol(dol)
    iso = args[0] if args else os.path.join(TEST_DIR, "Scooby-src.iso")
    os.makedirs(os.path.dirname(os.path.abspath(iso)), exist_ok=True)
    if not os.path.exists(iso) or os.path.getsize(iso) != os.path.getsize(ISO_IN):
        print("copying", ISO_IN, "->", iso)
        shutil.copyfile(ISO_IN, iso)
    patch_iso(iso, dol)
    print("test image:", iso)


if __name__ == "__main__":
    main()
