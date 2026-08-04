#!/usr/bin/env python3
"""Validate handwritten markdown examples that claim an output.

Rule enforced:
If a C++ code block is immediately followed by a `### Output` section, the code
block must contain an explicit print surface such as `std::cout` or
`pluss::OutEngine`/`out.writeln(...)`.

This is intentionally narrow. It does not try to prove that the printed output
matches the prose, only that the example actually emits something.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable


CODE_BLOCK_WITH_OUTPUT = re.compile(
    r"## Examples(?P<body>.*)", re.S
)
EXAMPLE_CPP_BLOCK = re.compile(
    r"```cpp\n(?P<code>.*?)```\n\n### Output",
    re.S,
)
PRINT_TOKENS = (
    "std::cout",
    ".writeln(",
    "OutEngine ",
    "OutEngine\n",
    "OutEngine\t",
)
SKIP_DIR_NAMES = {"_build", "_local", ".git", ".venv", "__pycache__"}


def iter_markdown_files(roots: Iterable[Path]) -> Iterable[Path]:
    for root in roots:
        if root.is_file():
            if root.suffix == ".md":
                yield root
            continue
        for path in root.rglob("*.md"):
            if any(part in SKIP_DIR_NAMES for part in path.parts):
                continue
            yield path


def validate_file(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    m = CODE_BLOCK_WITH_OUTPUT.search(text)
    if not m:
        return []

    body = m.group("body")
    failures: list[str] = []
    for i, block in enumerate(EXAMPLE_CPP_BLOCK.finditer(body), start=1):
        code = block.group("code")
        if not any(token in code for token in PRINT_TOKENS):
            failures.append(f"{path}: example {i} has `### Output` but no explicit print call")
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate markdown examples that claim an output.")
    parser.add_argument(
        "paths",
        nargs="+",
        help="markdown file(s) or directory roots to validate",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    roots = [Path(p).resolve() for p in args.paths]

    markdown_files = list(iter_markdown_files(roots))

    failures: list[str] = []
    for path in markdown_files:
        failures.extend(validate_file(path))

    if failures:
        print("doc example validation failed:", file=sys.stderr)
        for line in failures:
            print(f"  - {line}", file=sys.stderr)
        return 1

    print(f"doc example validation passed ({len(markdown_files)} markdown file(s) checked)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
