#!/usr/bin/env python3

###
# Prepares the DOL used for decomp-toolkit analysis.
#
# The retail DOL's entry point is a game function (a stack-fill routine in
# .text) that calls the real __start in .init. decomp-toolkit locates .init,
# _rom_copy_info and _bss_init_info relative to the entry point, so analysis
# fails on the retail DOL. This script extracts main.dol from the disc image
# and writes a copy with the entry point set to __start, used only for
# analysis. The build is still verified against the original DOL's hash.
#
# Usage:
#   python tools/prepare_dol.py <version dir, e.g. orig/GIHE78>
###

import hashlib
import struct
import sys
from pathlib import Path

ORIG_SHA1 = "c77fb29d39b5898933e8ef4d1ccd2ddac404554e"
ORIG_ENTRY = 0x80016898
START_ADDR = 0x80005304  # __start
DISC_EXTS = (".iso", ".gcm")


def dol_size(header: bytes) -> int:
    offsets = struct.unpack(">18I", header[0:72])
    sizes = struct.unpack(">18I", header[144:216])
    return max(o + s for o, s in zip(offsets, sizes) if s)


def extract_dol(disc: Path) -> bytes:
    with disc.open("rb") as f:
        f.seek(0x420)
        (dol_offset,) = struct.unpack(">I", f.read(4))
        f.seek(dol_offset)
        header = f.read(0x100)
        f.seek(dol_offset)
        return f.read(dol_size(header))


def main() -> None:
    base = Path(sys.argv[1] if len(sys.argv) > 1 else "orig/GIHE78")
    orig_dol = base / "sys" / "main.dol"
    out_dol = base / "main_analysis.dol"

    if not orig_dol.exists():
        discs = [p for p in base.iterdir() if p.suffix.lower() in DISC_EXTS]
        if not discs:
            sys.exit(f"No disc image (.iso/.gcm) or sys/main.dol found in {base}")
        data = extract_dol(discs[0])
        orig_dol.parent.mkdir(parents=True, exist_ok=True)
        orig_dol.write_bytes(data)

    data = bytearray(orig_dol.read_bytes())
    sha1 = hashlib.sha1(data).hexdigest()
    if sha1 != ORIG_SHA1:
        sys.exit(f"{orig_dol}: SHA-1 mismatch (got {sha1}, expected {ORIG_SHA1})")

    (entry,) = struct.unpack_from(">I", data, 0xE0)
    assert entry == ORIG_ENTRY, f"unexpected entry point {entry:#010x}"
    struct.pack_into(">I", data, 0xE0, START_ADDR)

    if not out_dol.exists() or out_dol.read_bytes() != data:
        out_dol.write_bytes(data)


if __name__ == "__main__":
    main()
