#!/usr/bin/env python3
"""Regenerates the `pages` lists in _static/api_nav.js from each family's landing-page toctree.

Run from this directory after adding or removing an API page:  python3 gen_api_nav.py
Only families listed in EXPAND get a page list; the others link to their landing page alone.
"""
import json
import re
from pathlib import Path

HERE = Path(__file__).parent
NAV = HERE / "_static" / "api_nav.js"
EXPAND = {"Core Utilities": "api/core", "Loaders": "api/loaders"}
TOCTREE = re.compile(r"```\{toctree\}\n(.*?)\n```", re.S)


def title(md: Path) -> str:
    first = md.read_text().split("\n", 1)[0].lstrip("# ").strip().replace("`", "")
    for prefix in ("pluss::tils::", "pluss::loader::", "pluss::"):
        if first.startswith(prefix):
            first = first[len(prefix):]
    return re.sub(r" API Reference$", "", first)


def walk(doc: str, seen: set):
    """Yields (label, html path) for every page reachable from doc's toctree, in toctree order."""
    md = HERE / f"{doc}.md"
    for block in TOCTREE.findall(md.read_text()):
        for line in block.splitlines():
            entry = line.strip()
            if not entry or entry.startswith(":"):
                continue
            target = str((Path(doc).parent / entry).as_posix())
            target = re.sub(r"[^/]+/\.\./", "", target)
            if target in seen or not (HERE / f"{target}.md").exists():
                continue
            seen.add(target)
            yield title(HERE / f"{target}.md"), f"{target}.html"
            yield from walk(target, seen)


def main():
    text = NAV.read_text()
    for label, landing in EXPAND.items():
        seen = {landing}
        pages = list(walk(landing, seen))
        body = "".join(
            f"        {{ label: {json.dumps(l)}, path: {json.dumps(p)} }},\n" for l, p in pages
        )
        pattern = re.compile(
            r'(label: "' + re.escape(label) + r'",\n\s+api: "[^"]+",\n\s+manual: "[^"]+",\n)(\s+pages: \[\n.*?\n\s+\],\n)?',
            re.S,
        )
        replacement = r"\1      pages: [\n" + body.replace("\\", "\\\\") + "      ],\n"
        text, n = pattern.subn(lambda m: m.group(1) + "      pages: [\n" + body + "      ],\n", text, count=1)
        assert n == 1, label
        print(f"{label}: {len(pages)} pages")
    NAV.write_text(text)


if __name__ == "__main__":
    main()
