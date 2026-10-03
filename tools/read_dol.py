#!/usr/bin/env python3
"""Reads values from the original DOL by virtual address.

Usage: python tools/read_dol.py <addr> [count] [f|w|h|b|s]
  f = float (default), w = u32, h = s16, b = bytes, s = C string
"""
import struct
import sys
from pathlib import Path

DOL = Path(__file__).resolve().parents[1] / "orig/GIHE78/sys/main.dol"


def sections(data):
    offs = struct.unpack(">18I", data[0:72])
    addrs = struct.unpack(">18I", data[72:144])
    sizes = struct.unpack(">18I", data[144:216])
    return [(a, o, s) for o, a, s in zip(offs, addrs, sizes) if s]


def read(data, addr, n):
    for a, o, s in sections(data):
        if a <= addr < a + s:
            return data[o + addr - a:o + addr - a + n]
    return None  # bss


def main():
    data = DOL.read_bytes()
    addr = int(sys.argv[1], 16)
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    kind = sys.argv[3] if len(sys.argv) > 3 else "f"
    if kind == "s":
        raw = read(data, addr, 256)
        print(raw.split(b"\0")[0].decode("latin-1"))
        return
    size = {"f": 4, "w": 4, "h": 2, "b": 1}[kind]
    raw = read(data, addr, size * count)
    if raw is None:
        print("not in an initialised section")
        return
    for i in range(count):
        chunk = raw[i * size:(i + 1) * size]
        if kind == "f":
            v = struct.unpack(">f", chunk)[0]
        elif kind == "w":
            v = "0x%08X" % struct.unpack(">I", chunk)[0]
        elif kind == "h":
            v = struct.unpack(">h", chunk)[0]
        else:
            v = "0x%02X" % chunk[0]
        print("%08X: %s" % (addr + i * size, v))


if __name__ == "__main__":
    main()
