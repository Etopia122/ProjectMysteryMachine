#!/usr/bin/env python3

###
# Compiler/flag probe: compiles a test source with many CodeWarrior versions
# and flag sets, and compares every function in the resulting object against
# the original DOL (relocated fields masked). Also reports the order in which
# the compiler emitted the functions.
#
# Target addresses are looked up by (mangled) name in config/<version>/symbols.txt.
#
# Usage:
#   python tools/compiler_probe/probe.py <version> <source.cpp> [--compilers 1.2.5,1.3.2] [--flags-file f.txt]
###

import argparse
import itertools
import re
import shlex
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Tuple

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parents[2]
RE_SYMBOL = re.compile(r"^(\S+) = \.\w+:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")

DEFAULT_COMPILERS = ["1.0", "1.1", "1.2.5", "1.2.5n", "1.3", "1.3.2", "2.0", "2.0p1", "2.5", "2.6", "2.7"]
BASE_FLAGS = [
    "-nodefaults", "-proc", "gekko", "-align", "powerpc", "-enum", "int", "-fp", "hardware",
    "-Cpp_exceptions", "off", "-RTTI", "off", "-c",
]
DEFAULT_VARIANTS = {
    "opt": [["-O4,p"], ["-O4,s"], ["-O3"]],
    "inline": [["-inline", "auto"], ["-inline", "deferred"], ["-inline", "auto,deferred"], ["-inline", "off"]],
}

# BFBB's game flags (cflags_bfbb minus inlining/opt, which are probed)
BFBB_FLAGS = [
    "-lang=c++", "-common", "on", "-char", "unsigned", "-str", "reuse,pool,readonly",
    "-use_lmw_stmw", "on", "-pragma", "cpp_extensions on", "-gccinc", "-fp_contract", "on",
    "-nosyspath", "-multibyte", "-DGAMECUBE",
]
BFBB_INCLUDES = [
    "include", "src/PowerPC_EABI_Support/include", "src/dolphin/include", "src/dolphin/src",
    "src/bink/include", "src", "include/inline", "include/rwsdk", "src/SB/Core/gc", "src/SB/Core/x",
    "src/SB/Game",
]

# Relocation type -> mask of the instruction bits it rewrites
R_PPC_ADDR16_LO, R_PPC_ADDR16_HI, R_PPC_ADDR16_HA, R_PPC_REL24, R_PPC_EMB_SDA21 = 4, 5, 6, 10, 109
RELOC_MASK = {
    R_PPC_ADDR16_LO: 0x0000FFFF,
    R_PPC_ADDR16_HI: 0x0000FFFF,
    R_PPC_ADDR16_HA: 0x0000FFFF,
    R_PPC_REL24: 0x03FFFFFC,
    R_PPC_EMB_SDA21: 0x001FFFFF,  # rA (r0 -> r13/r2) and offset
}


class Dol:
    def __init__(self, path: Path):
        d = path.read_bytes()
        self.data = d
        self.sections = [
            (o, a, s)
            for o, a, s in zip(
                struct.unpack(">18I", d[0:72]), struct.unpack(">18I", d[72:144]), struct.unpack(">18I", d[144:216])
            )
            if s
        ]

    def read(self, addr: int, size: int) -> bytes:
        for o, a, s in self.sections:
            if a <= addr < a + s:
                return self.data[o + addr - a : o + addr - a + size]
        raise ValueError(f"{addr:#x} not in DOL")


def load_targets(version: str) -> Dict[str, Tuple[int, int]]:
    out = {}
    for line in (ROOT / "config" / version / "symbols.txt").read_text().splitlines():
        m = RE_SYMBOL.match(line)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return out


def object_functions(path: Path):
    """Returns [(name, offset, bytes, {offset: mask})] in .text order."""
    with path.open("rb") as f:
        elf = ELFFile(f)
        text_idx = None
        text = None
        for i, sec in enumerate(elf.iter_sections()):
            if sec.name == ".text":
                text_idx, text = i, sec.data()
        if text is None:
            return []
        masks: Dict[int, int] = {}
        for sec in elf.iter_sections():
            if isinstance(sec, RelocationSection) and sec["sh_info"] == text_idx:
                for r in sec.iter_relocations():
                    t = r["r_info_type"]
                    masks[r["r_offset"]] = masks.get(r["r_offset"], 0) | RELOC_MASK.get(t, 0xFFFFFFFF)
        funcs = []
        symtab = elf.get_section_by_name(".symtab")
        for s in symtab.iter_symbols():
            if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] == text_idx:
                off, size = s["st_value"], s["st_size"]
                funcs.append((s.name, off, text[off : off + size], masks))
        funcs.sort(key=lambda x: x[1])
        return funcs


def compare(obj: bytes, orig: bytes, base: int, masks: Dict[int, int]) -> Tuple[int, int]:
    """Returns (matching instructions, total instructions)."""
    n = max(len(obj), len(orig)) // 4
    good = 0
    for i in range(min(len(obj), len(orig)) // 4):
        a = struct.unpack_from(">I", obj, i * 4)[0]
        b = struct.unpack_from(">I", orig, i * 4)[0]
        mask = ~masks.get(base + i * 4, 0) & 0xFFFFFFFF
        if a & mask == b & mask:
            good += 1
    return good, n


def print_diff(obj: bytes, orig: bytes, base: int, masks: Dict[int, int], addr: int) -> None:
    from capstone import CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN, Cs

    md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)

    def dis(code: bytes, start: int):
        out = []
        for i in range(0, len(code), 4):
            ins = next(md.disasm(code[i : i + 4], start + i), None)
            out.append(f"{ins.mnemonic} {ins.op_str}" if ins else ".word")
        return out

    a, b = dis(obj, 0), dis(orig, addr)
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else ""
        y = b[i] if i < len(b) else ""
        wa = struct.unpack_from(">I", obj, i * 4)[0] if i * 4 < len(obj) else None
        wb = struct.unpack_from(">I", orig, i * 4)[0] if i * 4 < len(orig) else None
        mask = ~masks.get(base + i * 4, 0) & 0xFFFFFFFF
        same = wa is not None and wb is not None and wa & mask == wb & mask
        print(f"{'  ' if same else '!!'} {x:36} | {y}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("version")
    ap.add_argument("source", type=Path)
    ap.add_argument("--compilers", default=",".join(DEFAULT_COMPILERS))
    ap.add_argument("--extra", default="", help="extra flags appended to every run")
    ap.add_argument("--variants", default="", help="semicolon-separated flag sets, replaces default variants")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--diff", default="", help="print a side-by-side diff of this function (first compiler/variant)")
    ap.add_argument("--bfbb", action="store_true", help="compile with BFBB's include paths (ref/bfbb-src)")
    args = ap.parse_args()

    targets = load_targets(args.version)
    dol = Dol(ROOT / "orig" / args.version / "sys" / "main.dol")
    include = ROOT / "tools" / "compiler_probe" / "include"

    if args.variants:
        variant_sets = [shlex.split(v) for v in args.variants.split(";")]
    else:
        variant_sets = [a + b for a, b in itertools.product(*DEFAULT_VARIANTS.values())]

    results = []
    with tempfile.TemporaryDirectory() as tmp:
        for comp in args.compilers.split(","):
            exe = ROOT / "build" / "compilers" / "GC" / comp / "mwcceppc.exe"
            if not exe.exists():
                print(f"missing compiler {comp}", file=sys.stderr)
                continue
            for variant in variant_sets:
                out = Path(tmp) / "probe.o"
                cmd = [str(exe), *BASE_FLAGS, *variant, *shlex.split(args.extra)]
                cwd = None
                if args.bfbb:
                    cwd = ROOT / "ref" / "bfbb-src"
                    cmd += BFBB_FLAGS
                    for inc in BFBB_INCLUDES:
                        cmd += ["-i", inc]
                else:
                    cmd += ["-i", str(include)]
                cmd += ["-o", str(out), str(args.source.resolve())]
                proc = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
                if proc.returncode != 0 or not out.exists():
                    if args.verbose:
                        print(f"{comp} {' '.join(variant)}: compile failed\n{proc.stdout}{proc.stderr}")
                    continue
                funcs = object_functions(out)
                if args.diff:
                    for name, off, code, masks in funcs:
                        if args.diff in name and name in targets:
                            addr, size = targets[name]
                            print_diff(code, dol.read(addr, size), off, masks, addr)
                    return
                total_good = total_n = 0
                detail = []
                for name, off, code, masks in funcs:
                    if name not in targets:
                        continue
                    addr, size = targets[name]
                    good, n = compare(code, dol.read(addr, size), off, masks)
                    if len(code) != size:
                        n = max(n, size // 4)
                    total_good += good
                    total_n += n
                    detail.append(f"{name}:{good}/{n}{'' if len(code) == size else f'(size {len(code):X}!={size:X})'}")
                order = [n for n, *_ in funcs]
                tgt = [n for n in order if n in targets]
                in_dol_order = sorted(tgt, key=lambda n: targets[n][0])
                order_tag = "same" if tgt == in_dol_order else "reversed" if tgt == in_dol_order[::-1] else "other"
                results.append((total_good / max(total_n, 1), comp, " ".join(variant), order_tag, detail))
                out.unlink()

    results.sort(key=lambda r: -r[0])
    for score, comp, variant, order_tag, detail in results[: 40 if not args.verbose else None]:
        print(f"{score * 100:6.2f}%  {comp:7} {variant:28} order:{order_tag:8} {' '.join(detail) if args.verbose else ''}")


if __name__ == "__main__":
    main()
