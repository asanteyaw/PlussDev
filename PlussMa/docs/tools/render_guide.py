#!/usr/bin/env python3
"""Renders PlussMa guide chapters from `*.md.in` templates.

A template is ordinary Markdown plus one marker line per example:

    @@example Numerical/optimization/guide/optima/01_first_problem.cpp@@

The marker is replaced by the program's source and the exact standard output of a
fresh build-and-run, so printed output in a chapter can never be typed by hand or
drift from the code. Paths are relative to PlussMa/Math/examples.

Usage:
    python3 PlussMa/docs/tools/render_guide.py            # render every template
    python3 PlussMa/docs/tools/render_guide.py --check    # fail if a rendered chapter is stale
    python3 PlussMa/docs/tools/render_guide.py FILE.md.in # render one template

Environment: PLUSSMA_GUIDE_COMPILE overrides the compile script
(default PlussTils/tools/verify_example.sh: `script SOURCE OUTPUT`).
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EXAMPLES = ROOT / "PlussMa" / "Math" / "examples"
COMPILE = Path(os.environ.get("PLUSSMA_GUIDE_COMPILE", ROOT / "PlussTils" / "tools" / "verify_example.sh"))
MARKER = re.compile(r"^@@example (\S+)@@\s*$", re.M)
BANNED = re.compile(r"\bteach\w*|\bmatlab\b|\bnumpy\b|\bpandas\b|\bpytorch\b|\bscikit-?learn\b|\bpython\b", re.I)
CACHE = Path(tempfile.gettempdir()) / "plussma_guide_cache"


def run_example(rel: str) -> tuple[str, str]:
    src = EXAMPLES / rel
    text = src.read_text()
    if BANNED.search(text):
        raise SystemExit(f"{rel}: banned wording in example source")
    key = hashlib.sha256((text + str(COMPILE.stat().st_mtime) + lib_stamp()).encode()).hexdigest()[:20]
    CACHE.mkdir(exist_ok=True)
    cached = CACHE / f"{key}.out"
    if cached.exists():
        return text, cached.read_text()
    exe = CACHE / f"{key}.exe"
    build = subprocess.run(["bash", str(COMPILE), str(src), str(exe)], capture_output=True, text=True)
    if build.returncode != 0:
        raise SystemExit(f"{rel}: compile failed\n{build.stderr[-3000:]}")
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300)
    if run.returncode != 0:
        raise SystemExit(f"{rel}: exit code {run.returncode}\n{run.stdout[-1500:]}\n{run.stderr[-1500:]}")
    cached.write_text(run.stdout)
    exe.unlink(missing_ok=True)
    return text, run.stdout


def lib_stamp() -> str:
    """Changes whenever the library or any public header an example can include changes."""
    lib = ROOT / "build-nn-optim-f64" / "libpluss.a"
    stamps = [str(lib.stat().st_mtime) if lib.exists() else "nolib"]
    for folder in (ROOT / "PlussMa" / "include", ROOT / "PlussQit" / "include", ROOT / "PlussMa" / "Math" / "Numerical"):
        newest = max((p.stat().st_mtime for p in folder.rglob("*") if p.suffix in {".h", ".cpp"}), default=0)
        stamps.append(str(newest))
    return "|".join(stamps)


def render(template: Path) -> str:
    body = template.read_text()
    if BANNED.search(body):
        m = BANNED.search(body)
        raise SystemExit(f"{template.name}: banned wording '{m.group(0)}'")

    def expand(match: re.Match) -> str:
        rel = match.group(1)
        source, output = run_example(rel)
        output = output.rstrip("\n")
        return f"```cpp\n{source.rstrip()}\n```\n\n### Output\n\n```text\n{output}\n```"

    return "<!-- generated from %s by PlussMa/docs/tools/render_guide.py; edit the template -->\n\n" % template.name + MARKER.sub(expand, body)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("templates", nargs="*")
    args = ap.parse_args()
    templates = [Path(t).resolve() for t in args.templates] or sorted((ROOT / "PlussMa").rglob("*.md.in"))
    stale = 0
    for t in templates:
        out = t.with_suffix("")  # strips .in -> .md
        rendered = render(t)
        if args.check:
            if not out.exists() or out.read_text() != rendered:
                print(f"STALE {out.relative_to(ROOT)}")
                stale += 1
        else:
            out.write_text(rendered)
            print(f"rendered {out.relative_to(ROOT)}")
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
