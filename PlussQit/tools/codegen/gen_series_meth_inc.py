#!/usr/bin/env python3
import argparse
import re
from pathlib import Path
from typing import List, Tuple, Dict

# ---------- C++ signature normalization helpers ----------

_ws = re.compile(r"\s+")

def _collapse_ws(s: str) -> str:
    return _ws.sub(" ", s).strip()

_CANON_MAP = [
    (re.compile(r"(?<!std::)\bint64_t\b"), "std::int64_t"),
    (re.compile(r"(?<!std::)\bint32_t\b"), "std::int32_t"),
    (re.compile(r"(?<!std::)\bint16_t\b"), "std::int16_t"),
    (re.compile(r"(?<!std::)\bint8_t\b"), "std::int8_t"),
    (re.compile(r"(?<!std::)\buint64_t\b"), "std::uint64_t"),
    (re.compile(r"(?<!std::)\buint32_t\b"), "std::uint32_t"),
    (re.compile(r"(?<!std::)\buint16_t\b"), "std::uint16_t"),
    (re.compile(r"(?<!std::)\buint8_t\b"), "std::uint8_t"),
]

def canonicalize_type_spelling(s: str) -> str:
    """Make common integer typedef spellings canonical for de-dup."""
    out = s
    for rx, repl in _CANON_MAP:
        out = rx.sub(repl, out)
    return out

def canonicalize_ref_ptr_spacing(s: str) -> str:
    """Canonicalize whitespace around C++ reference/pointer tokens.

    Ensures `T &` == `T&`, `T *` == `T*`, and similarly for rvalue refs.
    """
    out = _collapse_ws(s)
    # Handle rvalue refs first so we don't break '&&'
    out = re.sub(r"\s*&&\s*", "&&", out)
    # Then lvalue refs
    out = re.sub(r"\s*&\s*", "&", out)
    # Pointers
    out = re.sub(r"\s*\*\s*", "*", out)
    return out

def strip_default(param: str) -> str:
    # Remove default initializer (= ...), but respect templates by doing a simple split
    # at top-level '=' (not inside <...>, (...), {...}).
    depth_angle = depth_paren = depth_brace = 0
    for i, ch in enumerate(param):
        if ch == "<": depth_angle += 1
        elif ch == ">": depth_angle = max(0, depth_angle - 1)
        elif ch == "(": depth_paren += 1
        elif ch == ")": depth_paren = max(0, depth_paren - 1)
        elif ch == "{": depth_brace += 1
        elif ch == "}": depth_brace = max(0, depth_brace - 1)
        elif ch == "=" and depth_angle == 0 and depth_paren == 0 and depth_brace == 0:
            return param[:i].strip()
    return param.strip()

def split_params(param_list: str) -> List[str]:
    s = param_list.strip()
    if s == "" or s == "void":
        return []
    params = []
    start = 0
    depth_angle = depth_paren = depth_brace = 0
    for i, ch in enumerate(s):
        if ch == "<": depth_angle += 1
        elif ch == ">": depth_angle = max(0, depth_angle - 1)
        elif ch == "(": depth_paren += 1
        elif ch == ")": depth_paren = max(0, depth_paren - 1)
        elif ch == "{": depth_brace += 1
        elif ch == "}": depth_brace = max(0, depth_brace - 1)
        elif ch == "," and depth_angle == 0 and depth_paren == 0 and depth_brace == 0:
            params.append(s[start:i].strip())
            start = i + 1
    params.append(s[start:].strip())
    return [p for p in params if p]

def strip_param_name(param: str) -> str:
    """
    Best-effort removal of trailing parameter name, leaving just the type.
    Examples:
      'const Series& rhs' -> 'const Series&'
      'std::int64_t dim'  -> 'std::int64_t'
      'std::span<const std::int64_t> dims' -> 'std::span<const std::int64_t>'
      'bool keepdim' -> 'bool'
      'double b' -> 'double'
      'optional<DimnameList> names' -> 'optional<DimnameList>'
    Handles pointers/references attached to name: 'Series* x', 'Series &x', 'Series&& x'
    """
    p = _collapse_ws(param)
    p = strip_default(p)

    # If there's no space, it's probably already a type-only or something like "T&"
    if " " not in p:
        return canonicalize_ref_ptr_spacing(canonicalize_type_spelling(p))

    # Split tokens; keep everything except final identifier if it looks like a name.
    # We avoid stripping if the last token is something like 'const' or '&' or '*'
    tokens = p.split(" ")
    last = tokens[-1]

    # Last token must be an identifier (possibly with leading &/* attached)
    # e.g. "&rhs" or "*rhs" or "rhs"
    m = re.fullmatch(r"([*&]+)?([A-Za-z_]\w*)", last)
    if not m:
        return canonicalize_ref_ptr_spacing(canonicalize_type_spelling(p))  # can't confidently strip

    # Heuristic: if previous token ends with '>' or is a primitive/type keyword, it's likely a name.
    # We'll strip the last token.
    return canonicalize_ref_ptr_spacing(canonicalize_type_spelling(" ".join(tokens[:-1]).strip()))

def normalize_param_list(param_list: str) -> str:
    params = split_params(param_list)
    norm = [strip_param_name(p) for p in params]
    return ", ".join(canonicalize_ref_ptr_spacing(canonicalize_type_spelling(x)) for x in norm)

def make_sig_key(method_name: str, param_list: str, is_const: bool) -> str:
    # Torch-style: const is NOT part of overload key; all methods become const.
    npl = normalize_param_list(param_list)
    return f"{method_name}({npl})"

# ---------- Parsing ----------

DECL_RE = re.compile(
    r"""^\s*
        (?P<ret>[\w:\<\>\s,&\*\[\]]+?)\s+      # return type
        (?P<name>[A-Za-z_]\w*)\s*             # function name
        \(\s*(?P<params>.*?)\s*\)\s*          # params (non-greedy)
        (?P<const>const)?\s*;?\s*$            # optional const + optional ;
    """,
    re.VERBOSE,
)

def parse_method_decl_line(line: str) -> Tuple[str, str, str, bool]:
    """
    Returns (ret, name, params, is_const) or raises ValueError.
    """
    m = DECL_RE.match(line)
    if not m:
        raise ValueError("not a decl")
    ret = _collapse_ws(m.group("ret"))
    name = m.group("name")
    params = m.group("params") or ""
    is_const = (m.group("const") is not None)
    return ret, name, params, is_const

def read_candidates_from_dispatch(dispatch_path: Path) -> List[Tuple[str, str, str, bool]]:
    """
    Reads SeriesMethodsDispatch.inc-like definitions to infer signatures.
    Expected pattern: 'Series Series::foo(<params>) const { ... }'
    """
    out = []
    # Match: <ret> Series::name(params) [const] {
    DEF_RE = re.compile(
        r"""^\s*(?P<ret>[\w:\<\>\s,&\*\[\]]+?)\s+
             Series::(?P<name>[A-Za-z_]\w*)\s*
             \(\s*(?P<params>.*?)\s*\)\s*
             (?P<const>const)?\s*\{""",
        re.VERBOSE,
    )
    for raw in dispatch_path.read_text(encoding="utf-8").splitlines():
        m = DEF_RE.match(raw)
        if not m:
            continue
        ret = _collapse_ws(m.group("ret"))
        name = m.group("name")
        params = m.group("params") or ""
        is_const = (m.group("const") is not None)
        out.append((ret, name, params, is_const))
    return out

def read_candidates_from_methods_inc(methods_inc_path: Path) -> List[Tuple[str, str, str, bool]]:
    """
    If you already have a messy SeriesMethods.inc, we can parse decl lines from it.
    """
    out = []
    for raw in methods_inc_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        # Only consider lines that look like 'Series foo(... ) const;'
        try:
            ret, name, params, is_const = parse_method_decl_line(line)
            # Only member decls returning Series or Series& are relevant for our generator
            if ret.startswith("Series"):
                out.append((ret, name, params, is_const))
        except ValueError:
            pass
    return out

# ---------- Generation ----------

def generate_series_methods_inc(
    candidates: List[Tuple[str, str, str, bool]],
    out_path: Path,
    header_comment: str = "// @generated by tools/regenerate_series_methods_inc.py",
) -> None:
    # De-dup by C++ signature key (name + param types only, ignoring const)
    seen: Dict[str, Tuple[str, str, str, bool]] = {}
    for ret, name, params, is_const in candidates:
        # Torch-style: everything is const
        is_const = True
        key = make_sig_key(name, params, is_const)
        # Prefer keeping a version that includes defaults (for nicer API) IF it doesn't change signature.
        # We'll pick the "longer" params string as proxy for carrying defaults.
        prev = seen.get(key)
        if prev is None:
            seen[key] = (ret, name, params, is_const)
        else:
            if len(params) > len(prev[2]):
                seen[key] = (ret, name, params, is_const)

    # Sort by name then normalized signature
    items = list(seen.values())
    items.sort(key=lambda x: (x[1], make_sig_key(x[1], x[2], x[3])))

    lines: List[str] = [header_comment, ""]
    for ret, name, params, _ in items:
        lines.append(f"  {ret} {name}({params}) const;")
    lines.append("")
    out_path.write_text("\n".join(lines), encoding="utf-8")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-dispatch", type=str, default="", help="Path to SeriesMethodsDispatch.inc (to infer signatures)")
    ap.add_argument("--from-methods-inc", type=str, default="", help="Path to existing SeriesMethods.inc (to parse + dedup)")
    ap.add_argument("--out", type=str, required=True, help="Output path for SeriesMethods.inc")
    args = ap.parse_args()

    candidates: List[Tuple[str, str, str, bool]] = []

    if args.from_dispatch:
        candidates += read_candidates_from_dispatch(Path(args.from_dispatch))
    if args.from_methods_inc:
        candidates += read_candidates_from_methods_inc(Path(args.from_methods_inc))

    if not candidates:
        raise SystemExit("No candidates found. Provide --from-dispatch and/or --from-methods-inc.")

    out_path = Path(args.out)
    generate_series_methods_inc(candidates, out_path)
    print(f"Wrote {out_path} with {len(candidates)} input candidates deduped to member declarations.")

if __name__ == "__main__":
    main()