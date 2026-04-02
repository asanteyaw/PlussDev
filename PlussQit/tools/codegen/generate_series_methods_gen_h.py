#!/usr/bin/env python3
"""Generate CodeM/generated/SeriesMethodsGen.h from native_functions.yaml.

This emits a Torch-like method surface for `pluss::codem::Series` where each
method is a thin wrapper that forwards to `_compat::invoke` / `_compat::invoke_`.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


CPP_KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char8_t", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "consteval",
    "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
    "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "else", "enum", "explicit", "export", "extern", "false", "float", "for",
    "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
    "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "private", "protected", "public", "register", "reinterpret_cast", "requires",
    "return", "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "thread_local",
    "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
    "xor", "xor_eq",
}

EXTRA_METHOD_NAMES = {
    # MATLAB/NumPy-style extensions maintained by PlussQit.
    "dct", "idct", "dst", "idst", "hilbert", "histcounts",
    "skewness", "kurtosis", "nanvar", "nanstd", "ediff1d",
    "movsum", "movmean", "movmax", "movmin", "movstd", "movvar",
    "pdist2", "cdist", "normr", "normc", "standardize", "center",
    "besselj", "bessely", "besseli", "besselk", "beta", "betaln",
    "erfcinv", "erfcinv_", "zeta", "airy", "pdf", "cdf",
    "unwrap", "cplxpair", "fftshift", "ifftshift", "abs2",
}

# Linalg-style methods are intentionally hosted in pluss::subsp::math::linalg
# instead of the core Series method surface.
OUTSOURCED_LINALG_METHOD_NAMES = {
    "cholesky",
    "cholesky_inverse",
    "cholesky_solve",
    "det",
    "geqrf",
    "inverse",
    "logdet",
    "lu_solve",
    "matrix_exp",
    "matrix_power",
    "orgqr",
    "ormqr",
    "pinverse",
    "qr",
    "slogdet",
    "svd",
    "triangular_solve",
}

# Methods intentionally excluded from the public Series method surface.
EXCLUDED_METHOD_NAMES = {
    "values",
    "_values",
}


def parse_typed_methods_yaml(text: str) -> tuple[dict[str, list[dict[str, str]]], set[str]]:
    """Parse typed method wrappers + skip list from a tiny YAML subset.

    Expected structure:

      methods:
        - name: expand
          signature: "Series expand(pluss::msharp::RangeView::Int sizes, bool implicit=false) const"
          forward: "sizes, implicit"
        - name: reshape
          signature: "Series reshape(pluss::msharp::RangeView::Int shape) const"
          forward: "shape"

      skip_methods:
        - view
        - reshape
    """
    out: dict[str, list[dict[str, str]]] = {}
    skip_methods: set[str] = set()
    current: dict[str, str] | None = None
    in_skip_section = False

    def flush_current() -> None:
        nonlocal current
        if not current:
            return
        name = current.get("name", "").strip()
        sig = current.get("signature", "").strip()
        fwd = current.get("forward", "").strip()
        if name and sig:
            out.setdefault(name, []).append({"signature": sig, "forward": fwd})
        current = None

    for raw in text.splitlines():
        line = raw.rstrip()
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if re.match(r"^\s*skip_methods\s*:\s*$", line):
            flush_current()
            in_skip_section = True
            continue

        if in_skip_section and re.match(r"^\s*-\s*[A-Za-z_][A-Za-z0-9_]*\s*$", line):
            name = re.sub(r"^\s*-\s*", "", line).strip()
            if name and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                skip_methods.add(name)
            continue

        if re.match(r"^\s*methods\s*:\s*$", line):
            flush_current()
            in_skip_section = False
            continue

        if re.match(r"^\s*-\s*name\s*:", line):
            flush_current()
            current = {}
            in_skip_section = False
            m = re.match(r"^\s*-\s*name\s*:\s*(.+?)\s*$", line)
            if m:
                current["name"] = m.group(1).strip().strip("\"'")
            continue
        if current is None:
            continue
        m_sig = re.match(r"^\s*signature\s*:\s*(.+?)\s*$", line)
        if m_sig:
            current["signature"] = m_sig.group(1).strip().strip("\"'")
            continue
        m_fwd = re.match(r"^\s*forward\s*:\s*(.+?)\s*$", line)
        if m_fwd:
            current["forward"] = m_fwd.group(1).strip().strip("\"'")
            continue

    flush_current()
    return out, skip_methods


def extract_method_names(yaml_text: str) -> list[str]:
    funcs = re.findall(r"^\s*-\s*func:\s*([^\n#]+)$", yaml_text, flags=re.MULTILINE)
    names: set[str] = set()
    for schema in funcs:
        m = re.match(r"\s*([A-Za-z0-9_\.]+)\s*\(", schema)
        if not m:
            continue
        raw_name = m.group(1)
        name = raw_name.split(".", 1)[0]
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            continue
        if name in CPP_KEYWORDS:
            continue
        names.add(name)
    return sorted(names)


def extract_opsinternal_names(header_text: str) -> set[str]:
    names: set[str] = set()
    for m in re.finditer(r"^\s*(?:Series|Series&)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", header_text, flags=re.MULTILINE):
        name = m.group(1)
        if name not in CPP_KEYWORDS:
            names.add(name)
    return names


def extract_seed_list(path: Path) -> set[str]:
    names: set[str] = set()
    for raw in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        name = raw.strip()
        if not name:
            continue
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            continue
        if name in CPP_KEYWORDS:
            continue
        names.add(name)
    return names


def is_inplace(name: str) -> bool:
    return name.endswith("_") and not name.endswith("__")


def render_header(
    names: list[str],
    yaml_path: Path,
    typed_yaml_path: Path | None,
    typed_methods: dict[str, list[dict[str, str]]],
    skip_methods: set[str],
) -> str:
    lines: list[str] = []
    unresolved: list[str] = []
    lines.append("// @generated by tools/codegen/generate_series_methods_gen_h.py. Do not edit manually.")
    lines.append(f"// Source: {yaml_path.as_posix()}")
    if typed_yaml_path is not None:
        lines.append(f"// Typed signatures: {typed_yaml_path.as_posix()}")
    lines.append("#include <utility>")
    lines.append("")
    lines.append("  // --- Torch method-surface (CPU-only): generated typed wrappers ---")
    lines.append("  // Wrappers route through _compat::invoke/_compat::invoke_.")
    lines.append("  // Zero-fallback mode: every generated method must be concretely typed")
    lines.append("  // or explicitly listed under `skip_methods` in typed YAML.")
    lines.append("")

    for name in names:
        typed_overloads = typed_methods.get(name, [])
        if typed_overloads:
            lines.append(f"  // Typed wrappers for `{name}` (staged migration off variadic surface).")
            for entry in typed_overloads:
                sig = entry["signature"].strip()
                fwd = entry["forward"].strip()
                call = "invoke_" if is_inplace(name) else "invoke"
                if fwd:
                    lines.append(
                        f"  {sig} {{ return pluss::codem::_compat::{call}(\"{name}\", *this, {{{fwd}}}); }}"
                    )
                else:
                    lines.append(
                        f"  {sig} {{ return pluss::codem::_compat::{call}(\"{name}\", *this); }}"
                    )
                lines.append("")
            continue

        if name in skip_methods:
            lines.append(f"  // `{name}` has concrete declarations/definitions; variadic wrapper intentionally skipped.")
            lines.append("")
            continue

        unresolved.append(name)

    if unresolved:
        unresolved_list = ", ".join(unresolved)
        raise RuntimeError(
            "Series method generation failed in zero-fallback mode. "
            "The following methods have no typed wrapper and are not in skip_methods: "
            f"{unresolved_list}. "
            "Add typed signatures to series_methods_typed.yaml or explicitly add to skip_methods."
        )

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--yaml", required=True, help="Path to native_functions.yaml")
    ap.add_argument("--opsinternal", default="", help="Optional path to OpsInternal.h for extra method names")
    ap.add_argument("--seed-list", default="", help="Optional path to newline-separated method seed names")
    ap.add_argument(
        "--typed-yaml",
        default="",
        help="Optional YAML file describing staged concrete wrappers to emit instead of variadic templates",
    )
    ap.add_argument("--out", required=True, help="Output SeriesMethodsGen.h path")
    args = ap.parse_args()

    yaml_path = Path(args.yaml)
    out_path = Path(args.out)
    text = yaml_path.read_text(encoding="utf-8", errors="ignore")
    names = set(extract_method_names(text))
    names.update(EXTRA_METHOD_NAMES)
    if args.opsinternal:
        opsinternal_path = Path(args.opsinternal)
        if opsinternal_path.exists():
            names.update(extract_opsinternal_names(opsinternal_path.read_text(encoding="utf-8", errors="ignore")))
    if args.seed_list:
        seed_path = Path(args.seed_list)
        if seed_path.exists():
            names.update(extract_seed_list(seed_path))
    typed_yaml_path: Path | None = None
    typed_methods: dict[str, list[dict[str, str]]] = {}
    skip_methods: set[str] = set()
    if args.typed_yaml:
        typed_yaml_path = Path(args.typed_yaml)
        if typed_yaml_path.exists():
            typed_methods, skip_methods = parse_typed_methods_yaml(
                typed_yaml_path.read_text(encoding="utf-8", errors="ignore")
            )
            names.update(typed_methods.keys())
            names.update(skip_methods)
    names.difference_update(OUTSOURCED_LINALG_METHOD_NAMES)
    names.difference_update(EXCLUDED_METHOD_NAMES)
    names = sorted(names)
    rendered = render_header(names, yaml_path, typed_yaml_path, typed_methods, skip_methods)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
