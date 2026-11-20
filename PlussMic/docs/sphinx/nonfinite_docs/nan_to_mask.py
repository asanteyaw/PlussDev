#!/usr/bin/env python3
"""Rewrite doc examples that used a NaN as a stand-in for a missing cell so that they build a real missing cell instead.

Before the separation of NULL from NaN a floating NaN doubled as "missing". An example such as

    Shelf(pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}))

now builds a column holding a NaN VALUE. Where the example's point was a missing cell, this script rewrites the call to

    Shelf(pluss::make_series({1.0, 0.0, 3.0}), pluss::make_series(std::vector<bool>{true, false, true}))

(the 0.0 under a missing cell is only a placeholder). A bare `pluss::make_series(...)` handed straight to a DataFrame column or a
`{"name", ...}` pair is wrapped in `Shelf(...)` so the mask has somewhere to go. Calls that already pass a mask are left alone.

Usage: nan_to_mask.py PAGE.md ...      (rewrites in place; prints what changed)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

NAN_TOKENS = re.compile(r"^(?:std::numeric_limits<double>::quiet_NaN\(\)|pluss::series::nan\(\)|nan|nan_value|NAN|std::nan\(\"\"\))$")


def match_paren(text: str, start: int) -> int:
    """Index of the ')' that closes the '(' at text[start]."""
    depth = 0
    i = start
    in_str = False
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_top(items: str) -> list[str]:
    out, depth, cur = [], 0, []
    in_str = False
    for c in items:
        if in_str:
            cur.append(c)
            if c == '"':
                in_str = False
            continue
        if c == '"':
            in_str = True
            cur.append(c)
        elif c in "({[<":
            depth += 1
            cur.append(c)
        elif c in ")}]>":
            depth -= 1
            cur.append(c)
        elif c == "," and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    tail = "".join(cur)
    if tail.strip():
        out.append(tail)
    return out


def rewrite(code: str) -> str:
    out = []
    i = 0
    key = "pluss::make_series("
    while True:
        j = code.find(key, i)
        if j < 0:
            out.append(code[i:])
            break
        open_paren = j + len(key) - 1
        close = match_paren(code, open_paren)
        if close < 0:
            out.append(code[i:])
            break
        arg = code[open_paren + 1 : close]
        brace = arg.find("{")
        if brace < 0 or "quiet_NaN" not in arg and not re.search(r"\bnan(_value)?\b", arg):
            out.append(code[i : close + 1])
            i = close + 1
            continue
        lb, rb = brace, arg.rfind("}")
        prefix, inner, suffix = arg[:lb], arg[lb + 1 : rb], arg[rb + 1 :]
        elems = split_top(inner)
        flags = [bool(NAN_TOKENS.match(e.strip())) for e in elems]
        if not any(flags) or suffix.strip():
            out.append(code[i : close + 1])
            i = close + 1
            continue
        # already masked? ("Shelf(make_series(...), mask)") -> leave alone
        after = code[close + 1 :].lstrip()
        before = code[:j].rstrip()
        if before.endswith("Shelf(") and after.startswith(","):
            out.append(code[i : close + 1])
            i = close + 1
            continue
        new_elems = []
        for e, f in zip(elems, flags):
            if f:
                lead = e[: len(e) - len(e.lstrip())]
                trail = e[len(e.rstrip()) :]
                new_elems.append(lead + "0.0" + trail)
            else:
                new_elems.append(e)
        values = "pluss::make_series(" + prefix + "{" + ",".join(new_elems) + "}" + suffix + ")"
        mask = "pluss::make_series(std::vector<bool>{" + ", ".join("false" if f else "true" for f in flags) + "})"
        wrapped_in_shelf = before.endswith("Shelf(") and after.startswith(")")
        if wrapped_in_shelf:
            new = values + ", " + mask
        else:
            new = "Shelf(" + values + ", " + mask + ")"
        out.append(code[i:j])
        out.append(new)
        i = close + 1
    return "".join(out)


def drop_unused_nan_decl(code: str) -> str:
    for decl in (r"^[ \t]*(?:const )?(?:double|auto) nan(?:_value)? = std::numeric_limits<double>::quiet_NaN\(\);\n",):
        m = re.search(decl, code, re.M)
        if m:
            rest = code[: m.start()] + code[m.end():]
            if not re.search(r"\bnan(_value)?\b", rest):
                code = rest
    return code


def main() -> int:
    for page in sys.argv[1:]:
        p = Path(page)
        text = p.read_text(encoding="utf-8")
        if "## Examples" not in text:
            continue
        head, body = text.split("## Examples", 1)
        pieces = re.split(r"(```cpp\n.*?```\n)", body, flags=re.S)
        changed = 0
        for k, piece in enumerate(pieces):
            if piece.startswith("```cpp\n"):
                inner = piece[len("```cpp\n") : -len("```\n")]
                new = drop_unused_nan_decl(rewrite(inner))
                if new != inner:
                    changed += 1
                    pieces[k] = "```cpp\n" + new + "```\n"
        if changed:
            p.write_text(head + "## Examples" + "".join(pieces), encoding="utf-8")
        print(f"{page}: {changed} example(s) rewritten")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
