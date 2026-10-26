#!/usr/bin/env python3
"""Structural diff of one function: registers, branch targets and relocated
operands normalized, so only real instruction or ordering differences remain.

Usage: bash tools/diff_fn.sh <src> <fn> | python tools/struct_diff.py [--stack]
  --stack also hides stack offsets (frame layout differences).
"""
import difflib
import re
import sys

STACK = "--stack" in sys.argv


def norm(lines):
    out = []
    prev_lis = set()
    for t in lines:
        t = re.sub(r"-?0x[0-9a-f]+\((r2|r13)\)", "SYM", t)
        # sda21 address loads: unrelocated ours shows "li rX, 0"
        t = re.sub(r"^addi (r\d+), r(?:2|13), -?0x[0-9a-f]+$", r"li \1, 0", t)
        t = re.sub(r"\b0\(0\)", "SYM", t)
        if STACK:
            t = re.sub(r"-?(0x[0-9a-f]+|\d+)\(r1\)", "STK", t)
            t = re.sub(r"^addi (r\d+), r1, \S+$", r"addi \1, r1, STK", t)
        m = re.match(r"lis (r\d+), ", t)
        if m:
            prev_lis.add(m.group(1))
            t = f"lis {m.group(1)}, SYM"
        m = re.match(r"addi (r\d+), (r\d+), (\S+)$", t)
        if m and m.group(2) in prev_lis:
            prev_lis.discard(m.group(2))
            t = f"addi {m.group(1)}, {m.group(2)}, SYM"
        t = re.sub(r"\b[rf]\d+\b", "R", t)
        t = re.sub(r"\bcr\d\w*\b", "CR", t)
        t = re.sub(r"^(b\w*) .*", r"\1 L", t)
        out.append(t)
    return out


left, right = [], []
for line in sys.stdin:
    line = line.rstrip("\n")
    if "|" not in line or len(line) < 4:
        continue
    a, b = line[3:].split("|", 1)
    if a.strip():
        left.append(a.strip())
    if b.strip():
        right.append(b.strip())
for d in difflib.unified_diff(norm(left), norm(right), "ours", "target", n=2, lineterm=""):
    print(d)
