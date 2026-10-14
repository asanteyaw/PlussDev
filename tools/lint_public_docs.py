#!/usr/bin/env python3
"""Fails when hand-written public doc pages use banned wording or name other packages.

Usage: python3 tools/lint_public_docs.py [DIR ...]   (default: every library's docs sources)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT = ["PlussQit/docs", "PlussMic/docs", "PlussMa/docs", "PlussPl/docs", "PlussAI/docs", "PlussTils/docs", "docs"]
BANNED = re.compile(
    r"\bteach(?:es|ing|ings|er|erly)?\b|\bmatlab\b|\bnumpy\b|\bpandas\b|\bpytorch\b|\bscikit-?learn\b|\bsklearn\b|\bpython\b",
    re.I,
)
SKIP_PARTS = {"_build", "_local", "node_modules"}
EXEMPT_NAMES = {"apiref.py", "lint_public_docs.py"}
STANDARDS = re.compile(r"(standard|PLAN|REDESIGN)", re.I)


def main():
    roots = [ROOT / d for d in (sys.argv[1:] or DEFAULT)]
    bad = 0
    for root in roots:
        for path in root.rglob("*.md"):
            if SKIP_PARTS & set(path.parts) or path.name in EXEMPT_NAMES or STANDARDS.search(path.name):
                continue
            for n, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
                m = BANNED.search(line)
                if m:
                    bad += 1
                    print(f"{path.relative_to(ROOT)}:{n}: '{m.group(0)}'")
    print(f"{bad} banned-term hits")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
