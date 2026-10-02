#!/usr/bin/env python3

###
# Compiles library sources from a reference decomp and finds where each
# compiled object lands in our DOL (relocated fields masked).
#
# This is for libraries that are byte-identical across games (MetroTRK,
# Runtime, MSL, Dolphin SDK). A unit whose .text matches at one address can be
# copied over as-is; the match gives its exact split boundaries. Objects whose
# functions were partly removed by the linker's dead-stripping are placed as a
# "chain": the surviving functions in order and contiguous.
#
# Data sections are located through the object's .text relocations: each one
# is paired with the symbol dtk resolved at that instruction in our asm.
#
# Library definitions (compiler, flags, per-object extra flags) are read from
# the reference's configure.py, so they stay in sync with what matches there.
#
# Usage:
#   python tools/place_ref_objects.py <version> <ref root> <lib name> [<lib name> ...]
#       [--range 800DE530-800F391C] [--define VERSION=2] [--splits out.txt]
#
# Example (Wind Waker as reference):
#   python tools/place_ref_objects.py GIHE78 ref/tww-src TRK_MINNOW_DOLPHIN Runtime.PPCEABI.H MSL_C
###

import argparse
import ast
import re
import shlex
import struct
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parents[1]

R_PPC_ADDR32, R_PPC_ADDR16_LO, R_PPC_ADDR16_HI, R_PPC_ADDR16_HA = 1, 4, 5, 6
R_PPC_REL24, R_PPC_REL14, R_PPC_EMB_SDA21 = 10, 11, 109
RELOC_MASK = {
    R_PPC_ADDR32: 0xFFFFFFFF,
    R_PPC_ADDR16_LO: 0x0000FFFF,
    R_PPC_ADDR16_HI: 0x0000FFFF,
    R_PPC_ADDR16_HA: 0x0000FFFF,
    R_PPC_REL24: 0x03FFFFFC,
    R_PPC_REL14: 0x0000FFFC,
    R_PPC_EMB_SDA21: 0x001FFFFF,
}

RE_ASM_LINE = re.compile(r"^/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t(.*)$")
RE_ASM_REF = re.compile(r'"?([^"\s,()]+?)"?(?:\+0x([0-9A-Fa-f]+))?@(ha|h|l|sda21)')
RE_SYM = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+);")

Funcs = List[Tuple[int, int, str]]


class Dol:
    def __init__(self, path: Path):
        d = path.read_bytes()
        self.data = d
        offs = struct.unpack(">18I", d[0:72])
        addrs = struct.unpack(">18I", d[72:144])
        sizes = struct.unpack(">18I", d[144:216])
        self.text = [(o, a, s) for i, (o, a, s) in enumerate(zip(offs, addrs, sizes)) if s and i < 7]

    def region(self, start: int, end: int) -> bytes:
        for o, a, s in self.text:
            if a <= start and end <= a + s:
                return self.data[o + start - a : o + end - a]
        raise ValueError(f"{start:#x}-{end:#x} not in one text section")


def parse_ref_configure(path: Path) -> Dict[str, dict]:
    """Extracts {lib: {mw_version, cflags, objects: [(status, path, extra)]}}."""
    text = path.read_text()
    libs = {}
    for m in re.finditer(r'\{\s*"lib":\s*"([^"]+)",(.*?)"objects":\s*\[(.*?)\]\s*,?\s*\}', text, re.S):
        name, head, body = m.group(1), m.group(2), m.group(3)
        mw = re.search(r'"mw_version":\s*"([^"]+)"', head)
        cf = re.search(r'"cflags":\s*([\w\[\]*,\s"\-]+?),\s*\n', head)
        objs = []
        for om in re.finditer(r'Object\(\s*([\w]+(?:\([^)]*\))?)\s*,\s*"([^"]+)"(.*?)\)\s*,', body):
            extra = []
            em = re.search(r"extra_cflags=(\[[^\]]*\])", om.group(3))
            if em:
                extra = ast.literal_eval(em.group(1))
            objs.append((om.group(1), om.group(2), extra))
        libs[name] = {"mw_version": mw.group(1) if mw else None, "cflags": cf.group(1).strip() if cf else None, "objects": objs}
    # helper functions, e.g. def DolphinLib(lib_name, objects): return {...}
    for hm in re.finditer(r"^def (\w+)\(lib_name[^)]*\)[^:]*:\s*return \{(.*?)\}", text, re.S | re.M):
        helper, body = hm.group(1), hm.group(2)
        mw = re.search(r'"mw_version":\s*"([^"]+)"', body)
        cf = re.search(r'"cflags":\s*(\w+)', body)
        for cm in re.finditer(rf'\b{helper}\(\s*"([^"]+)",\s*\[(.*?)\]\s*,?\s*\)', text, re.S):
            objs = []
            for om in re.finditer(r'Object\(\s*([\w]+(?:\([^)]*\))?)\s*,\s*"([^"]+)"(.*?)\)\s*,', cm.group(2)):
                extra = []
                em = re.search(r"extra_cflags=(\[[^\]]*\])", om.group(3))
                if em:
                    extra = ast.literal_eval(em.group(1))
                objs.append((om.group(1), om.group(2), extra))
            libs[cm.group(1)] = {"mw_version": mw.group(1) if mw else None, "cflags": cf.group(1) if cf else None, "objects": objs}
    return libs


def eval_cflags(path: Path, name: str, defines: List[str]) -> List[str]:
    """Evaluates a cflags list (e.g. cflags_trk) from the reference configure.py."""
    text = path.read_text()

    def get(var: str) -> List[str]:
        m = re.search(rf"^{var}\s*=\s*\[(.*?)^\]", text, re.S | re.M)
        if not m:
            raise ValueError(f"{var} not found")
        out: List[str] = []
        for line in m.group(1).splitlines():
            line = line.split("#", 1)[0].strip().rstrip(",")
            if not line:
                continue
            if line.startswith("*"):
                out += get(line[1:])
            elif line.startswith(("f\"", "f'")):
                continue  # version/build-specific paths
            else:
                out.append(ast.literal_eval(line))
        return out

    if name.startswith("["):  # inline list, e.g. [*cflags_base, "-O3"]
        raw: List[str] = []
        for item in name.strip("[]").split(","):
            item = item.strip()
            if item.startswith("*"):
                raw += get(item[1:])
            elif item:
                raw.append(ast.literal_eval(item))
    else:
        raw = get(name)
    flags = [f for f in raw if not f.startswith("-DVERSION")]
    flags += ["-DNDEBUG=1"] + [f"-D{d}" for d in defines]
    return [word for f in flags for word in shlex.split(f)]


def object_text(path: Path) -> Tuple[bytes, Dict[int, int], Funcs]:
    with path.open("rb") as f:
        elf = ELFFile(f)
        sections = list(elf.iter_sections())
        idx = next((i for i, s in enumerate(sections) if s.name == ".text"), None)
        if idx is None:
            return b"", {}, []
        text = sections[idx].data()
        masks: Dict[int, int] = {}
        for sec in sections:
            if isinstance(sec, RelocationSection) and sec["sh_info"] == idx:
                for r in sec.iter_relocations():
                    at = r["r_offset"] & ~3  # 16-bit fields point at the instruction's low half
                    masks[at] = masks.get(at, 0) | RELOC_MASK.get(r["r_info_type"], 0xFFFFFFFF)
        funcs = []
        for s in elf.get_section_by_name(".symtab").iter_symbols():
            if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] == idx:
                funcs.append((s["st_value"], s["st_size"], s.name))
        return text, masks, sorted(funcs)


def find(text: bytes, masks: Dict[int, int], hay: bytes) -> List[int]:
    n = len(text) // 4
    words = struct.unpack(f">{n}I", text)
    keep = [~masks.get(i * 4, 0) & 0xFFFFFFFF for i in range(n)]
    # anchor on the first run of 4 unrelocated words
    anchor = next((i for i in range(n - 3) if all(keep[i + k] == 0xFFFFFFFF for k in range(4))), None)
    hits = []
    if anchor is None:
        candidates = range(0, len(hay) - len(text) + 1, 4)
    else:
        pat = text[anchor * 4 : anchor * 4 + 16]
        candidates = []
        pos = hay.find(pat)
        while pos != -1:
            if pos % 4 == 0 and pos - anchor * 4 >= 0:
                candidates.append(pos - anchor * 4)
            pos = hay.find(pat, pos + 4)
    for start in candidates:
        if start + len(text) > len(hay):
            continue
        hw = struct.unpack_from(f">{n}I", hay, start)
        if all((a & k) == (b & k) for a, b, k in zip(words, hw, keep)):
            hits.append(start)
    return hits


def func_matches_at(text: bytes, masks: Dict[int, int], off: int, size: int, hay: bytes, pos: int) -> bool:
    if size == 0 or pos < 0 or pos + size > len(hay):
        return False
    n = size // 4
    a = struct.unpack_from(f">{n}I", text, off)
    b = struct.unpack_from(f">{n}I", hay, pos)
    for i in range(n):
        k = ~masks.get(off + i * 4, 0) & 0xFFFFFFFF
        if (a[i] & k) != (b[i] & k):
            return False
    return True


def place_chain(text, masks, funcs: Funcs, hay) -> Optional[Tuple[int, int, Dict[str, int]]]:
    """Places the object's functions in order, allowing functions the linker
    stripped to be missing. Anchors on each uniquely-found function and grows a
    contiguous chain in both directions; returns the longest one (by bytes)."""
    funcs = [f for f in funcs if f[1] > 0]
    best = None
    for ai, (off, size, name) in enumerate(funcs):
        sub = text[off : off + size]
        submask = {k - off: v for k, v in masks.items() if off <= k < off + size}
        hits = find(sub, submask, hay)
        if len(hits) != 1:
            continue
        s = e = hits[0]
        placed = {name: s}
        e += size
        for off2, size2, name2 in funcs[ai + 1 :]:
            if func_matches_at(text, masks, off2, size2, hay, e):
                placed[name2] = e
                e += size2
        for off2, size2, name2 in reversed(funcs[:ai]):
            if func_matches_at(text, masks, off2, size2, hay, s - size2):
                s -= size2
                placed[name2] = s
        if best is None or e - s > best[1] - best[0]:
            best = (s, e, placed)
    return best


def load_asm_refs(version: str) -> Dict[int, Tuple[str, int]]:
    """{instruction address: (symbol, addend)} for data references in dtk's asm."""
    refs: Dict[int, Tuple[str, int]] = {}
    for path in (ROOT / "build" / version / "asm").rglob("*.s"):
        for line in path.read_text(errors="ignore").splitlines():
            m = RE_ASM_LINE.match(line)
            if not m:
                continue
            r = RE_ASM_REF.search(m.group(2))
            if r:
                refs[int(m.group(1), 16)] = (r.group(1), int(r.group(2), 16) if r.group(2) else 0)
    return refs


def load_symbols(version: str) -> Dict[str, int]:
    out = {}
    for line in (ROOT / "config" / version / "symbols.txt").read_text().splitlines():
        m = RE_SYM.match(line)
        if m:
            out[m.group(1)] = int(m.group(3), 16)
    return out


def place_data(obj: Path, func_addr: Dict[int, int], funcs: Funcs, refs, symbols) -> Dict[str, Tuple[Optional[int], int, str]]:
    """For each allocated non-.text section of the object: (base address or None, size, note).
    func_addr maps object .text function offsets to their DOL addresses."""
    out: Dict[str, Tuple[Optional[int], int, str]] = {}
    with obj.open("rb") as f:
        elf = ELFFile(f)
        sections = list(elf.iter_sections())
        text_idx = next(i for i, sec in enumerate(sections) if sec.name == ".text")
        data_secs = {
            i: sec
            for i, sec in enumerate(sections)
            if sec["sh_flags"] & 2 and sec.name not in (".text", ".init") and sec["sh_size"] > 0
        }
        syms = list(elf.get_section_by_name(".symtab").iter_symbols())
        votes: Dict[int, List[int]] = {i: [] for i in data_secs}
        for rs in sections:
            if not isinstance(rs, RelocationSection) or rs["sh_info"] != text_idx:
                continue
            for r in rs.iter_relocations():
                sym = syms[r["r_info_sym"]]
                shndx = sym["st_shndx"]
                if not isinstance(shndx, int) or shndx not in data_secs:
                    continue
                at = r["r_offset"] & ~3
                fn = next((fo for fo, sz, _ in funcs if fo <= at < fo + sz), None)
                if fn is None or fn not in func_addr:
                    continue
                ref = refs.get(func_addr[fn] + at - fn)
                if not ref or ref[0] not in symbols:
                    continue
                target = (0 if sym["st_info"]["type"] == "STT_SECTION" else sym["st_value"]) + r["r_addend"]
                votes[shndx].append(symbols[ref[0]] + ref[1] - target)
        # fallback for sections .text doesn't reference: named symbols that
        # already exist in our symbols.txt
        for s in syms:
            shndx = s["st_shndx"]
            if isinstance(shndx, int) and shndx in data_secs and not votes[shndx]:
                if s.name and s["st_info"]["type"] in ("STT_OBJECT", "STT_NOTYPE") and s.name in symbols:
                    votes[shndx].append(symbols[s.name] - s["st_value"])
        for i, sec in data_secs.items():
            v = set(votes[i])
            if len(v) == 1:
                out[sec.name] = (v.pop(), sec["sh_size"], "")
            elif not v:
                out[sec.name] = (None, sec["sh_size"], "no references from .text")
            else:
                out[sec.name] = (None, sec["sh_size"], "conflicting: " + " ".join(f"{x:08X}" for x in sorted(v)))
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("version")
    ap.add_argument("ref", type=Path)
    ap.add_argument("libs", nargs="+")
    ap.add_argument("--range", default="800DE530-800F391C")
    ap.add_argument("--define", action="append", default=["VERSION=2"])
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "tmp" / "ref_objects")
    ap.add_argument("--splits", type=Path, help="write splits.txt entries for placed objects here")
    ap.add_argument("--prefix", default="", help="prefix for unit names in the written splits")
    ap.add_argument("--renames", type=Path, help="write ADDR=name lines (for tools/rename.py) for placed functions")
    args = ap.parse_args()

    ref = args.ref.resolve()
    start, end = (int(x, 16) for x in args.range.split("-"))
    dol = Dol(ROOT / "orig" / args.version / "sys" / "main.dol")
    hay = dol.region(start, end)
    libs = parse_ref_configure(ref / "configure.py")
    refs = load_asm_refs(args.version)
    symbols = load_symbols(args.version)
    split_out: List[str] = []
    renames: List[str] = []

    for lib in args.libs:
        cfg = libs[lib]
        flags = eval_cflags(ref / "configure.py", cfg["cflags"], args.define)
        exe = ROOT / "build" / "compilers" / "GC" / cfg["mw_version"].split("/")[1] / "mwcceppc.exe"
        print(f"## {lib} ({cfg['mw_version']}, {cfg['cflags']})")
        for status, src, extra in cfg["objects"]:
            if not src.endswith((".c", ".cp", ".cpp")):
                print(f"   skip (asm)  {src}")
                continue
            srcpath = ref / "src" / src
            out = args.out / lib / (src + ".o")
            out.parent.mkdir(parents=True, exist_ok=True)
            cmd = [str(exe), *flags, *[x for e in extra for x in shlex.split(e)], "-c", "-o", str(out), str(srcpath)]
            if sys.platform != "win32":
                cmd.insert(0, "wibo")
            proc = subprocess.run(cmd, capture_output=True, text=True, cwd=ref)
            if proc.returncode != 0:
                msg = (proc.stdout + proc.stderr).strip().splitlines()
                print(f"   COMPILE ERR {src}: {msg[-1] if msg else ''}")
                continue
            text, masks, funcs = object_text(out)
            if not text:
                print(f"   (no .text)  {src}")
                continue
            hits = find(text, masks, hay)
            ref_status = status.split("(")[0]
            placement = None
            if len(hits) == 1:
                addr = start + hits[0]
                placement = (addr, addr + len(text), {fo: addr + fo for fo, _, _ in funcs}, 0)
                print(f"   MATCH {addr:08X}-{addr + len(text):08X} {len(funcs):3} fns  [{ref_status}] {src}")
            elif hits:
                print(f"   MULTI ({len(hits)})  {src}")
            else:
                chain = place_chain(text, masks, funcs, hay)
                if chain:
                    s, e, placed = chain
                    stripped = len({f[2] for f in funcs}) - len(placed)
                    offs = {fo: start + placed[n] for fo, _, n in funcs if n in placed}
                    placement = (start + s, start + e, offs, stripped)
                    print(
                        f"   CHAIN {start + s:08X}-{start + e:08X} {len(placed):3} fns, {stripped} stripped  "
                        f"[{ref_status}] {src}"
                    )
                else:
                    print(f"   none  [{ref_status}] {src}")
            if placement:
                ts, te, offs, stripped = placement
                for fo, _, fname in funcs:
                    if fo in offs and not fname.startswith(("@", ".")):
                        renames.append(f"{offs[fo]:08X}={fname}")
                data = place_data(out, offs, funcs, refs, symbols)
                for name, (b, size, note) in data.items():
                    where = f"{b:08X}-{b + size:08X}" if b is not None else "?"
                    print(f"         {name:8} {size:5X} @ {where} {note}")
                # sizes of data sections are only certain when nothing was stripped
                ok = all(b is not None for b, _, _ in data.values()) and not (stripped and data)
                if args.splits:
                    lines = [f"{args.prefix}{src}:", f"\t.text       start:0x{ts:08X} end:0x{te:08X}"]
                    for name, (b, size, _) in data.items():
                        if b is not None:
                            lines.append(f"\t{name:<11} start:0x{b:08X} end:0x{b + size:08X}")
                    split_out.append(("" if ok else "# UNCERTAIN\n") + "\n".join(lines))
    if args.splits:
        args.splits.write_text("\n\n".join(split_out) + "\n", newline="\n")
    if args.renames:
        args.renames.write_text("\n".join(renames) + "\n", newline="\n")


if __name__ == "__main__":
    main()
