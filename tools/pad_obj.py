#!/usr/bin/env python3
# Write a copy of a source-built object whose data sections are padded back to
# the sizes of the original unit, so that every following (original) unit keeps
# its original address and alignment. Used for --non-matching builds: DMA
# buffers and DSP code in the original units must stay 32-byte aligned.
#
# The padding is appended to each section and the section's last symbol is
# grown to cover it, so the linker's dead-stripping keeps it.
#
# usage: pad_obj.py <built.o> <original.o> <out.o> [adjust.json]
import json
import os
import struct
import sys

SECTIONS = [".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2"]
NOBITS_TYPE = 8


def read_elf(path):
    d = bytearray(open(path, "rb").read())
    shoff = struct.unpack(">I", d[0x20:0x24])[0]
    es, n, si = struct.unpack(">HHH", d[0x2E:0x34])
    sh = [list(struct.unpack(">IIIIIIIIII", d[shoff + i * es:shoff + (i + 1) * es])) for i in range(n)]
    shstr = sh[si][4]
    names = [d[shstr + s[0]:d.index(b"\0", shstr + s[0])].decode() for s in sh]
    return d, sh, names


def section_sizes(path):
    d, sh, names = read_elf(path)
    out = {}
    for s, name in zip(sh, names):
        if name in SECTIONS:
            out[name] = out.get(name, 0) + s[5]
    return out


def main():
    built, orig, out = sys.argv[1:4]
    adjust = {}
    if len(sys.argv) > 4 and os.path.exists(sys.argv[4]):
        adjust = json.load(open(sys.argv[4])).get(os.path.basename(out), {})
    ours = section_sizes(built)
    theirs = section_sizes(orig)

    d, sh, names = read_elf(built)
    pads = {}
    for i, name in enumerate(names):
        if name not in SECTIONS:
            continue
        diff = theirs.get(name, 0) - ours.get(name, 0)
        if diff < 0:
            diff %= 32  # larger than the original: at least keep the 32-byte phase
        diff += adjust.get(name, 0)
        diff %= 0x10000
        if diff > 0:
            pads[i] = diff

    # grow the last symbol of each padded section
    for si_, s in enumerate(sh):
        if s[1] != 2:
            continue
        best = {}
        for j in range(s[5] // 16):
            off = s[4] + j * 16
            nm, val, sz, info, oth, shn = struct.unpack(">IIIBBH", d[off:off + 16])
            if shn in pads and (info & 0xF) in (0, 1):
                if shn not in best or val + sz > best[shn][1]:
                    best[shn] = (off, val + sz)
        for shn, (off, end) in best.items():
            val, sz = struct.unpack(">II", d[off + 4:off + 12])
            struct.pack_into(">I", d, off + 8, sz + pads[shn] + (sh[shn][5] - end))

    # rebuild the file with padded section contents
    body = bytearray(d[:0x34])
    for i, s in enumerate(sh):
        if i == 0:
            continue
        data = bytes(d[s[4]:s[4] + s[5]]) if s[1] != NOBITS_TYPE else b""
        if i in pads:
            if s[1] != NOBITS_TYPE:
                data += b"\0" * pads[i]
            s[5] += pads[i]
        align = max(s[8], 4)
        while len(body) % align:
            body.append(0)
        s[4] = len(body)
        body += data
    while len(body) % 4:
        body.append(0)
    shoff = len(body)
    for s in sh:
        body += struct.pack(">IIIIIIIIII", *s)
    struct.pack_into(">I", body, 0x20, shoff)
    open(out, "wb").write(body)


if __name__ == "__main__":
    main()
