#!/usr/bin/env python3
# Build the game from the decompiled sources (--non-matching, linking every
# unit marked Equivalent) and write the resulting main.dol into a copy of the
# original disc image, for testing in Dolphin or on hardware.
#
# usage: python tools/make_test_iso.py [--no-build] [out.iso]
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


def build_source_dol(dst):
    run(sys.executable, "configure.py", "--non-matching")
    try:
        run("ninja")
        shutil.copyfile(DOL_OUT, dst)
    finally:
        run(sys.executable, "configure.py")


def patch_iso(iso, dol):
    data = open(dol, "rb").read()
    with open(iso, "r+b") as f:
        f.seek(0x420)
        dol_off, fst_off = struct.unpack(">II", f.read(8))
        room = fst_off - dol_off
        if len(data) > room:
            raise SystemExit("main.dol is 0x%X bytes, only 0x%X fit before the FST" % (len(data), room))
        f.seek(dol_off)
        f.write(data)
        f.write(b"\0" * (room - len(data)))
    print("wrote 0x%X-byte main.dol at 0x%X (room 0x%X)" % (len(data), dol_off, room))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    os.makedirs(TEST_DIR, exist_ok=True)
    dol = os.path.join(TEST_DIR, "main.dol")
    if "--no-build" not in sys.argv:
        build_source_dol(dol)
    iso = args[0] if args else os.path.join(TEST_DIR, "Scooby-src.iso")
    if not os.path.exists(iso) or os.path.getsize(iso) != os.path.getsize(ISO_IN):
        print("copying", ISO_IN, "->", iso)
        shutil.copyfile(ISO_IN, iso)
    patch_iso(iso, dol)
    print("test image:", iso)


if __name__ == "__main__":
    main()
