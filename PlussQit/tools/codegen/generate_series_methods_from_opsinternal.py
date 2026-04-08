#!/usr/bin/env python3
"""Generate Series member declarations from CodeM/core/OpsInternal.cpp.

This script extracts out-of-class `Series::foo(...)` definitions and emits a
declaration include consumed by `Series.h`, keeping declarations in sync with
the evolving implementation surface.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


STATIC_METHODS = {
    "undefined",
    "empty",
    "zeros",
    "ones",
    "full",
    "arange",
    "linspace",
    "eye",
    "rand",
    "randn",
    "from_storage",
}


SERIES_DECL_RE = re.compile(
    r"(?m)^\s*(static\s+)?(Series(?:\s*&)?(?:\s+const)?)\s+([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"\(([^)]*)\)\s*(const)?\s*;"
)

SERIES_DEF_RE = re.compile(
    r"(?m)^\s*(Series(?:\s*&)?(?:\s+const)?)\s+Series::([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"\(([^)]*)\)\s*(const)?\s*\{"
)


def _split_top_level(s: str, sep: str) -> list[str]:
    out: list[str] = []
    cur: list[str] = []
    ang = par = brk = brc = 0
    for ch in s:
        if ch == "<":
            ang += 1
        elif ch == ">" and ang > 0:
            ang -= 1
        elif ch == "(":
            par += 1
        elif ch == ")" and par > 0:
            par -= 1
        elif ch == "[":
            brk += 1
        elif ch == "]" and brk > 0:
            brk -= 1
        elif ch == "{":
            brc += 1
        elif ch == "}" and brc > 0:
            brc -= 1

        if ch == sep and ang == 0 and par == 0 and brk == 0 and brc == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    out.append("".join(cur))
    return out


def _strip_default(param: str) -> str:
    ang = par = brk = brc = 0
    for i, ch in enumerate(param):
        if ch == "<":
            ang += 1
        elif ch == ">" and ang > 0:
            ang -= 1
        elif ch == "(":
            par += 1
        elif ch == ")" and par > 0:
            par -= 1
        elif ch == "[":
            brk += 1
        elif ch == "]" and brk > 0:
            brk -= 1
        elif ch == "{":
            brc += 1
        elif ch == "}" and brc > 0:
            brc -= 1
        elif ch == "=" and ang == 0 and par == 0 and brk == 0 and brc == 0:
            return param[:i].strip()
    return param.strip()


def _canonical_param_type(param: str) -> str:
    p = _strip_default(param)
    if not p:
        return ""
    tokens = p.split()
    if len(tokens) >= 2 and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", tokens[-1]):
        # Drop trailing parameter name (`foo`) while preserving the full type.
        idx = p.rfind(tokens[-1])
        if idx > 0:
            p = p[:idx].rstrip()
    return " ".join(p.split())


def _signature_key(
    static_prefix: str,
    ret: str,
    name: str,
    params: str,
    const_qual: str,
) -> str:
    plist = []
    raw = params.strip()
    if raw:
        for p in _split_top_level(raw, ","):
            pt = _canonical_param_type(p)
            if pt:
                plist.append(pt)
    sp = "static " if static_prefix else ""
    cq = " const" if const_qual else ""
    return f"{sp}{' '.join(ret.split())} {name}({','.join(plist)}){cq}"


def parse_existing_decls(series_header: Path) -> set[str]:
    text = series_header.read_text(encoding="utf-8")
    keys: set[str] = set()
    for m in SERIES_DECL_RE.finditer(text):
        key = _signature_key(
            static_prefix=(m.group(1) or "").strip(),
            ret=m.group(2),
            name=m.group(3),
            params=m.group(4),
            const_qual=(m.group(5) or "").strip(),
        )
        keys.add(key)
    return keys


def extract_series_decls(opsinternal_cpp: Path, existing_keys: set[str]) -> list[str]:
    text = opsinternal_cpp.read_text(encoding="utf-8")
    out: list[str] = []
    seen: set[str] = set()
    for m in SERIES_DEF_RE.finditer(text):
        ret = " ".join(m.group(1).split())
        name = m.group(2)
        params = " ".join(m.group(3).split())
        qual = " const" if m.group(4) else ""
        static_prefix = "static " if name in STATIC_METHODS else ""
        decl = f"{static_prefix}{ret} {name}({params}){qual};"
        key = _signature_key(static_prefix, ret, name, params, qual)
        if key in existing_keys:
            continue
        if key not in seen:
            seen.add(key)
            out.append(decl)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--opsinternal", required=True)
    ap.add_argument("--series-header", required=True)
    ap.add_argument("--out-decls", required=True)
    ap.add_argument("--out-defs", required=True)
    args = ap.parse_args()

    out_decls = Path(args.out_decls)
    out_defs = Path(args.out_defs)
    opsinternal_h = Path(args.opsinternal)
    opsinternal_cpp = opsinternal_h.with_suffix(".cpp")
    out_decls.parent.mkdir(parents=True, exist_ok=True)

    existing = parse_existing_decls(Path(args.series_header))
    decls = extract_series_decls(opsinternal_cpp, existing)
    out_decls.write_text(
        "// AUTO-GENERATED. Do not edit.\n"
        "// Extracted from CodeM/core/OpsInternal.cpp Series:: definitions.\n\n"
        + "\n".join(decls)
        + ("\n" if decls else ""),
        encoding="utf-8",
    )
    out_defs.write_text(
        "// AUTO-GENERATED. Do not edit.\n"
        "// No out-of-line wrappers emitted in this pass.\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
