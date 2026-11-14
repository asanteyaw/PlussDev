#!/usr/bin/env python3
"""Renders PlussMa guide chapters from `*.md.in` templates.

A template is ordinary Markdown plus one marker line per example:

    @@example Numerical/optimization/guide/optima/01_first_problem.cpp@@

The marker is replaced by the program's source and the exact standard output of a
fresh build-and-run, so printed output in a chapter can never be typed by hand or
drift from the code. Paths are relative to PlussMa/Math/examples.

An example may write figures (for instance SVG files) into the directory named by the environment variable PLUSSMA_GUIDE_ASSETS; they are
copied to an `assets` folder next to the template, so a chapter can embed them as `![caption](assets/name.svg)`.

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
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EXAMPLES = ROOT / "PlussMa" / "Math" / "examples"
COMPILE = Path(os.environ.get("PLUSSMA_GUIDE_COMPILE", ROOT / "PlussTils" / "tools" / "verify_example.sh"))
MARKER = re.compile(r"^@@example (\S+)@@\s*$", re.M)
# A page may carry its own program: the fenced cpp block that follows an `@@run@@` line is compiled and run, and its output is placed
# beneath it, so the code a reader sees is exactly the code that produced the output.
RUN = re.compile(r"^@@run@@\s*\n```cpp\n(.*?)\n```[ \t]*$", re.M | re.S)
BANNED = re.compile(r"\bteach\w*|\bmatlab\b|\bnumpy\b|\bpandas\b|\bpytorch\b|\bscikit-?learn\b|\bpython\b", re.I)
CACHE = Path(tempfile.gettempdir()) / "plussma_guide_cache"


def run_example(rel: str, assets: Path | None = None) -> tuple[str, str]:
    src = EXAMPLES / rel
    text = src.read_text()
    if BANNED.search(text):
        raise SystemExit(f"{rel}: banned wording in example source")
    key = hashlib.sha256((text + str(COMPILE.stat().st_mtime) + lib_stamp()).encode()).hexdigest()[:20]
    CACHE.mkdir(exist_ok=True)
    cached = CACHE / f"{key}.out"
    kept = CACHE / f"{key}.assets"

    def publish_assets() -> None:
        # Files an example writes into $PLUSSMA_GUIDE_ASSETS (figures) are placed next to the chapter that embeds them.
        if assets is not None and kept.exists():
            assets.mkdir(parents=True, exist_ok=True)
            for f in kept.iterdir():
                shutil.copy2(f, assets / f.name)

    if cached.exists():
        publish_assets()
        return text, cached.read_text()
    exe = CACHE / f"{key}.exe"
    build = subprocess.run(["bash", str(COMPILE), str(src), str(exe)], capture_output=True, text=True)
    if build.returncode != 0:
        raise SystemExit(f"{rel}: compile failed\n{build.stderr[-3000:]}")
    shutil.rmtree(kept, ignore_errors=True)
    kept.mkdir()
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300, env={**os.environ, "PLUSSMA_GUIDE_ASSETS": str(kept)})
    if run.returncode != 0:
        raise SystemExit(f"{rel}: exit code {run.returncode}\n{run.stdout[-1500:]}\n{run.stderr[-1500:]}")
    cached.write_text(run.stdout)
    exe.unlink(missing_ok=True)
    publish_assets()
    return text, run.stdout


def run_source(text: str, label: str, assets: Path | None = None) -> str:
    if BANNED.search(text):
        raise SystemExit(f"{label}: banned wording in example source")
    key = hashlib.sha256((text + str(COMPILE.stat().st_mtime) + lib_stamp()).encode()).hexdigest()[:20]
    CACHE.mkdir(exist_ok=True)
    cached = CACHE / f"{key}.out"
    kept = CACHE / f"{key}.assets"

    def publish_assets() -> None:
        if assets is not None and kept.exists() and any(kept.iterdir()):
            assets.mkdir(parents=True, exist_ok=True)
            for f in kept.iterdir():
                shutil.copy2(f, assets / f.name)

    if cached.exists():
        publish_assets()
        return cached.read_text()
    src = CACHE / f"{key}.cpp"
    src.write_text(text + "\n")
    exe = CACHE / f"{key}.exe"
    build = subprocess.run(["bash", str(COMPILE), str(src), str(exe)], capture_output=True, text=True)
    if build.returncode != 0:
        raise SystemExit(f"{label}: compile failed\n{build.stdout[-1500:]}\n{build.stderr[-3000:]}")
    shutil.rmtree(kept, ignore_errors=True)
    kept.mkdir()
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300, env={**os.environ, "PLUSSMA_GUIDE_ASSETS": str(kept)})
    if run.returncode != 0:
        raise SystemExit(f"{label}: exit code {run.returncode}\n{run.stdout[-1500:]}\n{run.stderr[-1500:]}")
    cached.write_text(run.stdout)
    exe.unlink(missing_ok=True)
    publish_assets()
    return run.stdout


def lib_stamp() -> str:
    """Changes whenever the library or any public header an example can include changes.

    PLUSSMA_GUIDE_STAMP pins the stamp while a large set of pages is being written, so unchanged programs are not recompiled after every
    library edit; render without it for the final pass."""
    pinned = os.environ.get("PLUSSMA_GUIDE_STAMP")
    if pinned:
        return pinned
    lib = Path(os.environ.get("PLUSSDEV_BUILD", str(ROOT / "build-nn-optim-f64"))) / "libpluss.a"
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
        source, output = run_example(rel, template.parent / "assets")
        # The explanatory comment block at the top of an example file is prose for the maintainer; the page carries its own prose, so the
        # shown program starts at its first #include (the comment changes nothing about what the program does).
        lines = source.split("\n")
        i = 0
        while i < len(lines) and lines[i].startswith("//"):
            i += 1
        source = "\n".join(lines[i:])
        output = output.rstrip("\n")
        return f"```cpp\n{source.rstrip()}\n```\n\nOutput\n\n```text\n{output}\n```"

    def expand_run(match: re.Match) -> str:
        code = match.group(1)
        output = run_source(code, template.name, template.parent / "assets").rstrip("\n")
        return f"```cpp\n{code}\n```\n\nOutput\n\n```text\n{output}\n```"

    body = RUN.sub(expand_run, body)
    return "<!-- generated from %s by PlussMa/docs/tools/render_guide.py; edit the template -->\n\n" % template.name + MARKER.sub(expand, body)


GENERATED_BLOCKS = [("<!-- apiref:nav:start -->", "<!-- apiref:nav:end -->"), ("<!-- apiref:toctree:start -->", "<!-- apiref:toctree:end -->")]


def strip_generated_blocks(text: str) -> str:
    for start, end in GENERATED_BLOCKS:
        text = re.sub(re.escape(start) + r".*?" + re.escape(end) + r"\n*", "", text, flags=re.S)
    return re.sub(r"\n{3,}", "\n\n", text).strip()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("templates", nargs="*")
    args = ap.parse_args()
    templates = [Path(t).resolve() for t in args.templates] or sorted((ROOT / "PlussMa").rglob("*.md.in"))
    # Compile and run every program the templates need in parallel first (results are cached), so the rendering pass below only reads the cache.
    jobs = []
    for t in templates:
        body = t.read_text()
        for m in RUN.finditer(body):
            jobs.append(("run", m.group(1), (t.name, t.parent / "assets"), t))
        for m in MARKER.finditer(body):
            jobs.append(("example", m.group(1), t.parent / "assets", t))
    failed: dict[Path, str] = {}
    if jobs:
        from concurrent.futures import ThreadPoolExecutor

        def work(job):
            kind, payload, extra, template = job
            try:
                return run_source(payload, extra[0], extra[1]) if kind == "run" else run_example(payload, extra)
            except BaseException as exc:  # one broken program must not stop the others from being built
                failed.setdefault(template, str(exc))

        with ThreadPoolExecutor(max_workers=int(os.environ.get("PLUSSMA_GUIDE_JOBS", "6"))) as pool:
            list(pool.map(work, jobs))
    stale = 0
    for t in templates:
        if t in failed:
            print(f"FAILED {t.relative_to(ROOT)}\n{failed[t][:1500]}")
            stale += 1
            continue
        out = t.with_suffix("")  # strips .in -> .md
        rendered = render(t)
        if args.check:
            # the navigation and toctree blocks are written into the page by `apiref.py wire`, not by the template, so they are not compared
            if not out.exists() or strip_generated_blocks(out.read_text()) != strip_generated_blocks(rendered):
                print(f"STALE {out.relative_to(ROOT)}")
                stale += 1
        else:
            out.write_text(rendered)
            print(f"rendered {out.relative_to(ROOT)}")
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
