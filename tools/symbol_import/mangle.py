#!/usr/bin/env python3

###
# CodeWarrior C++ name mangling for function signatures as printed by
# `dtk dwarf dump`, e.g.
#   "signed int xSerial::Read(float * buf)"  ->  "Read__7xSerialFPf"
#   "void xBaseInit(class xBase * xb, class p2BaseAsset * asset)"
#                                            ->  "xBaseInit__FP5xBaseP11p2BaseAsset"
###

import re
from typing import List, Tuple

BUILTINS = {
    "void": "v",
    "bool": "b",
    "char": "c",
    "signed char": "Sc",
    "unsigned char": "Uc",
    "signed short": "s",
    "short": "s",
    "unsigned short": "Us",
    "signed int": "i",
    "int": "i",
    "unsigned int": "Ui",
    "signed long": "l",
    "long": "l",
    "unsigned long": "Ul",
    "signed long long": "x",
    "long long": "x",
    "unsigned long long": "Ux",
    "float": "f",
    "double": "d",
    "long double": "r",
    "wchar_t": "w",
}

OPERATORS = {
    "operator=": "__as",
    "operator+": "__pl",
    "operator-": "__mi",
    "operator*": "__ml",
    "operator/": "__dv",
    "operator+=": "__apl",
    "operator-=": "__ami",
    "operator*=": "__amu",
    "operator/=": "__adv",
    "operator==": "__eq",
    "operator!=": "__ne",
    "operator<": "__lt",
    "operator>": "__gt",
    "operator[]": "__vc",
    "operator()": "__cl",
    "operator new": "__nw",
    "operator delete": "__dl",
}


class MangleError(Exception):
    pass


def split_params(s: str) -> List[str]:
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def matching_paren(s: str, open_idx: int) -> int:
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == "(":
            depth += 1
        elif s[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    raise MangleError(f"unbalanced parens: {s}")


TAG_KEYWORDS = ("class", "struct", "union", "enum")


def strip_param_name(t: str) -> str:
    """Removes a trailing parameter name, e.g. 'class xBase * xb' -> 'class xBase *'."""
    t = t.strip()
    if t.endswith("*") or t.endswith("&"):
        return t
    m = re.match(r"^(.*[\*&])\s*\w+$", t)
    if m:
        return m.group(1).strip()
    tokens = t.split()
    if tokens and tokens[0] in TAG_KEYWORDS:
        return " ".join(tokens[:2])
    if t in BUILTINS:
        return t
    if " ".join(tokens[:-1]) in BUILTINS:
        return " ".join(tokens[:-1])
    return t


def mangle_type(t: str) -> str:
    t = t.strip()
    # Function pointer: "<ret> (* [name])(<params>)" or array pointer "<elem> (* [name])[N]"
    m = re.match(r"^(.*?)\(\s*(\*+)\s*\w*\s*\)\s*(\(.*\)|\[\d+\])$", t)
    if m:
        ret, stars, tail = m.group(1), m.group(2), m.group(3)
        prefix = "P" * (len(stars) - 1)
        if tail.startswith("["):
            return prefix + "PA" + tail[1:-1] + "_" + mangle_type(ret)
        params = mangle_params(tail[1:-1])
        return prefix + "PF" + params + "_" + mangle_type(ret)
    t = strip_param_name(t)
    if t.endswith("*"):
        return "P" + mangle_type(t[:-1])
    if t.endswith("&"):
        return "R" + mangle_type(t[:-1])
    if t.startswith("const "):
        return "C" + mangle_type(t[6:])
    if t.startswith("volatile "):
        return "V" + mangle_type(t[9:])
    if t in BUILTINS:
        return BUILTINS[t]
    m = re.match(r"^(?:class|struct|union|enum) (\w+)$", t)
    if m:
        name = m.group(1)
        return f"{len(name)}{name}"
    if t == "...":
        return "e"
    raise MangleError(f"unsupported type: {t!r}")


def mangle_params(params: str) -> str:
    ps = split_params(params)
    if not ps or ps == ["void"]:
        return "v"
    return "".join(mangle_type(p) for p in ps)


def parse_signature(sig: str) -> Tuple[str, str, str]:
    """Returns (qualified name, params string, trailing qualifiers)."""
    sig = sig.strip()
    if sig.startswith("static "):
        sig = sig[7:]
    # Find the parameter list belonging to the function name: the last
    # top-level "(...)" (a function returning a function pointer nests it).
    close = sig.rindex(")")
    depth = 0
    open_idx = -1
    for i in range(close, -1, -1):
        if sig[i] == ")":
            depth += 1
        elif sig[i] == "(":
            depth -= 1
            if depth == 0:
                open_idx = i
                break
    head = sig[:open_idx].strip()
    params = sig[open_idx + 1 : close]
    trailing = sig[close + 1 :].strip()
    if "operator" in head:
        name = head[head.index("operator") :]
        before = head[: head.index("operator")]
        qual = before.split()[-1] if before.split() and before.split()[-1].endswith("::") else ""
        return qual + name, params, trailing
    return head.split()[-1].lstrip("*&"), params, trailing


def mangle(sig: str, is_c: bool = False) -> str:
    qname, params, trailing = parse_signature(sig)
    parts = qname.split("::")
    name = parts[-1]
    classes = parts[:-1]
    if is_c or (not classes and name == "main"):
        return name
    if name in OPERATORS:
        name = OPERATORS[name]
    elif classes and name == classes[-1]:
        name = "__ct"
    elif classes and name == "~" + classes[-1]:
        name = "__dt"
    if len(classes) == 0:
        qual = ""
    elif len(classes) == 1:
        qual = f"{len(classes[0])}{classes[0]}"
    else:
        qual = f"Q{len(classes)}" + "".join(f"{len(c)}{c}" for c in classes)
    const = "C" if trailing.startswith("const") else ""
    return f"{name}__{qual}{const}F{mangle_params(params)}"


if __name__ == "__main__":
    tests = [
        ("signed int xSerial::Read(float * buf)", "Read__7xSerialFPf"),
        ("void xBaseInit(class xBase * xb, class p2BaseAsset * asset)", "xBaseInit__FP5xBaseP11p2BaseAsset"),
        ("signed int xSerialShutdown()", "xSerialShutdown__Fv"),
        ("void xSerialTraverse(signed int (* func)(unsigned int, class xSerial *))", "xSerialTraverse__FPFUiP7xSerial_i"),
        ("signed int xSerial::Read(char * buf, signed int elesize, signed int n)", "Read__7xSerialFPcii"),
        ("static signed int xSER_ord_test(void * key, void * elt)", "xSER_ord_test__FPvPv"),
        ("signed int xSerial_svgame_register(class st_XSAVEGAME_DATA * sgctxt, enum en_SAVEGAME_MODE mode)",
         "xSerial_svgame_register__FP17st_XSAVEGAME_DATA16en_SAVEGAME_MODE"),
        ("void foo(float (* N)[4])", "foo__FPA4_f"),
    ]
    for sig, want in tests:
        got = mangle(sig)
        print(("ok  " if got == want else "FAIL") + f" {got}" + ("" if got == want else f"  (want {want})"))
