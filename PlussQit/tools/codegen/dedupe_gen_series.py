#!/usr/bin/env python3
import re
import sys
from pathlib import Path

# Matches:   <ret> <name>(<params>) const;
DECL_RE = re.compile(
    r"^\s*(?P<ret>.+?)\s+(?P<name>[A-Za-z_]\w*)\s*\((?P<params>.*)\)\s+const\s*;\s*$"
)

def canon_ws(s: str) -> str:
    s = re.sub(r"\s+", " ", s.strip())
    # Canonicalize spacing around &&, &, *
    s = re.sub(r"\s*&&\s*", "&&", s)
    s = re.sub(r"\s*&\s*", "&", s)
    s = re.sub(r"\s*\*\s*", "*", s)
    return s

def split_params(s: str):
    s = s.strip()
    if not s:
        return []
    out, cur, depth = [], [], 0
    for ch in s:
        if ch in "<({[":
            depth += 1
        elif ch in ">)}]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    out.append("".join(cur).strip())
    return out

def drop_default(p: str) -> str:
    # Remove " = ..." at top level (good enough for this generated file)
    return re.sub(r"\s*=\s*.*$", "", p).strip()

def canon_int_typedefs(p: str) -> str:
    # normalize int typedef spellings
    reps = [
        ("int64_t",  "std::int64_t"),
        ("int32_t",  "std::int32_t"),
        ("int16_t",  "std::int16_t"),
        ("int8_t",   "std::int8_t"),
        ("uint64_t", "std::uint64_t"),
        ("uint32_t", "std::uint32_t"),
        ("uint16_t", "std::uint16_t"),
        ("uint8_t",  "std::uint8_t"),
    ]
    for a, b in reps:
        p = re.sub(rf"(?<!std::)\b{a}\b", b, p)
    return p

def strip_param_name(p: str) -> str:
    p = canon_ws(canon_int_typedefs(drop_default(p)))

    # Attempt to drop trailing identifier if it looks like a param name.
    # Example: "const Series& rhs" -> "const Series&"
    toks = p.split(" ")
    if len(toks) >= 2:
        last = toks[-1]
        if re.match(r"^[A-Za-z_]\w*$", last):
            # keep everything except last token
            p = " ".join(toks[:-1]).strip()
            p = canon_ws(p)

    return p

def signature_key(line: str):
    m = DECL_RE.match(line)
    if not m:
        return None
    name = m.group("name")
    params = m.group("params")

    ptypes = []
    for p in split_params(params):
        ptypes.append(strip_param_name(p))

    # IMPORTANT:
    # - const is fixed in your API, ignore it for dedupe
    # - return type is NOT part of overload resolution, ignore it too
    return f"{name}({','.join(ptypes)})"

def dedupe_file(path: Path) -> int:
    lines = path.read_text().splitlines(True)

    seen = set()
    kept = []
    dropped = []

    for i, line in enumerate(lines, 1):
        k = signature_key(line)
        if k is None:
            kept.append(line)
            continue
        if k in seen:
            dropped.append((i, line.rstrip("\n"), k))
            continue
        seen.add(k)
        kept.append(line)

    path.write_text("".join(kept))

    if dropped:
        print(f"[dedupe] dropped {len(dropped)} duplicate declarations:")
        for i, text, k in dropped[:200]:
            print(f"  line {i}: {text}   -> {k}")
        if len(dropped) > 200:
            print(f"  ... and {len(dropped)-200} more")
    else:
        print("[dedupe] no duplicates found")

    return len(dropped)

def main():
    if len(sys.argv) != 2:
        print("usage: dedupe_series_methods_inc.py <SeriesMethods.inc>", file=sys.stderr)
        sys.exit(2)

    path = Path(sys.argv[1])
    if not path.exists():
        print(f"error: file not found: {path}", file=sys.stderr)
        sys.exit(1)

    dedupe_file(path)

if __name__ == "__main__":
    main()