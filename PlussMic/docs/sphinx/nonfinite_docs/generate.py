#!/usr/bin/env python3
"""Generate the NULL / NaN / infinity API reference pages from real runs.

For every (type, method) pair this script
  1. builds the examples described in spec.py,
  2. compiles ONE program per type that runs every example of that type and prints each result between markers,
  3. runs it and captures the output verbatim,
  4. writes `api/<type>/<method>.md` as a hand-authored page (marker + signature hash, so generate_api_reference.py leaves it alone),
  5. keeps the landing-page toctrees, every page's previous/next footer and the sidebar manifest in step.

Usage:  generate.py [--types Shelf,DataFrame] [--methods isnan,dropnan] [--no-run]
Nothing in a page is typed by hand except the prose in spec.py: every printed output below an example is the program's output.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent))

import generate_api_reference as g  # noqa: E402
from fixtures import BY_NAME, TYPES  # noqa: E402
import spec  # noqa: E402

ROOT = g.ROOT
DOCS = g.DOCS
VERIFY = ROOT / "PlussTils/tools/verify_example.sh"
import tempfile
WORK = pathlib.Path(tempfile.gettempdir()) / "pluss_nonfinite_docs"

HEADERS = {"Nexus": ROOT / "PlussMic/src/bundle/Nexus.h", "Locus": ROOT / "PlussMic/src/bundle/Locus.h"}
INDEX = {"Nexus": DOCS / "nexus_api_reference.md", "Locus": DOCS / "locus_api_reference.md"}


def methods_for(t_name: str) -> list[str]:
    names = spec.FLAG_NAMES + spec.DROP_NAMES + spec.FILL_NAMES
    if t_name in ("DataFrame", "Catalog"):
        names = names + ["count_kinds"]
    if t_name == "Shelf":
        names = names + ["nnull", "nnan", "nposinf", "nneginf"]
    if t_name in ("Atlas", "Nexus", "Locus"):
        names = names + spec.NULL_FAMILY
    return names


def header_for(cls: str) -> pathlib.Path:
    cfg = g.CLASS_CONFIG.get(cls)
    return cfg["header"] if cfg else HEADERS[cls]


def index_for(cls: str) -> pathlib.Path:
    cfg = g.CLASS_CONFIG.get(cls)
    return cfg["output"] if cfg else INDEX[cls]


def signature_block(cls: str, name: str) -> tuple[list[str], str]:
    ms = [m for m in g.extract_methods(cls, header_for(cls)) if m.name == name]
    if not ms:
        raise RuntimeError(f"{cls}::{name} not found in the header")
    sigs = []
    for m in ms:
        s = g.user_signature(m, cls)
        if m.static:
            s = "static " + s
        elif re.search(r"\)\s*const\b", m.signature):
            s += " const"
        if m.signature.startswith("template"):
            tmpl = m.signature.split(">", 1)[0] + ">"
            s = tmpl + "\n" + s
        sigs.append(s)
    return sigs, g.compute_signature_hash(ms)


# ------------------------------------------------------------------------------------------------------------------------------
# Running the examples
# ------------------------------------------------------------------------------------------------------------------------------

def program_for(t_name: str, jobs: list[tuple[str, int, dict]]) -> str:
    out = ["#include <pluss/pluss.h>", "#include <iostream>", "#include <limits>", "#include <string>", "#include <vector>", "",
           "int main() {"]
    for method, idx, ex in jobs:
        out.append(f'  std::cout << "@@BEGIN {method} {idx}\\n";')
        out.append("  try {")
        for line in ex["code"]:
            out.append("    " + line if line else "")
        out.append("  } catch (const std::exception& e) {")
        out.append('    std::cout << "EXCEPTION: " << e.what() << "\\n";')
        out.append("  }")
        out.append(f'  std::cout << "\\n@@END\\n";')
    out.append("  return 0;")
    out.append("}")
    return "\n".join(out) + "\n"


def run_type(t_name: str, jobs, verbose=True) -> dict[tuple[str, int], str]:
    WORK.mkdir(exist_ok=True)
    src = WORK / f"ex_{t_name}.cpp"
    exe = WORK / f"ex_{t_name}"
    src.write_text(program_for(t_name, jobs))
    r = subprocess.run([str(VERIFY), str(src), str(exe)], capture_output=True, text=True)
    if r.returncode != 0 or not exe.exists():
        sys.stderr.write(r.stdout[-4000:] + r.stderr[-6000:])
        raise SystemExit(f"compile failed for {t_name}")
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    if run.returncode != 0:
        sys.stderr.write(run.stderr[-2000:])
        raise SystemExit(f"run failed for {t_name} (exit {run.returncode})")
    results: dict[tuple[str, int], str] = {}
    cur = None
    buf: list[str] = []
    for line in run.stdout.splitlines():
        m = re.match(r"@@BEGIN (\w+) (\d+)$", line)
        if m:
            cur = (m.group(1), int(m.group(2)))
            buf = []
        elif line == "@@END":
            results[cur] = "\n".join(buf).strip("\n").rstrip()
            cur = None
        elif cur is not None:
            buf.append(line.rstrip())
    return results


# ------------------------------------------------------------------------------------------------------------------------------
# Writing pages
# ------------------------------------------------------------------------------------------------------------------------------

def write_page(t_name: str, name: str, ps: dict, outputs: dict[int, str], sigs: list[str], sig_hash: str) -> pathlib.Path:
    t = BY_NAME[t_name]
    index_stem = index_for(t_name).stem
    L: list[str] = ["<!-- pluss-hand-authored -->", f"<!-- pluss-signature-hash: {sig_hash} -->", "", f"# {t_name}::{name}", "",
                    f"[Back to {t_name} API Reference](../../{index_stem})", ""]
    L += ["## Signatures" if len(sigs) > 1 else "## Signature", "", "```cpp"]
    for i, s in enumerate(sigs):
        if i:
            L.append("")
        L += s.splitlines()
    L += ["```", "", "## What It Does", ""]
    L += ps["what"]
    L += ["", "## Parameters", ""] + ps["params"]
    L += ["## Returns", ""] + ps["returns"].splitlines() + [""]
    L += ["## Examples", ""]
    for i, ex in enumerate(ps["examples"], start=1):
        L += [f"Example {i}. {ex['title']}", "", "```cpp"] + ex["code"] + ["```", "", "### Output", "", "```text"]
        L += (outputs.get(i - 1, "").splitlines() or ["(no output)"])
        L += ["```", "", ex["expl"], ""]
    L += ["## Notes", ""]
    for n in ps["notes"]:
        L.append("- " + n.replace("\n", "\n  "))
    L += ["", "## See Also", ""] + ps["see_also"] + [""]
    path = DOCS / "api" / t.dirname / f"{name}.md"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(L) + "\n", encoding="utf-8")
    return path


# ------------------------------------------------------------------------------------------------------------------------------
# Navigation: toctree, previous/next, sidebar manifest
# ------------------------------------------------------------------------------------------------------------------------------

PREV_NEXT_RE = re.compile(r'\n*<div class="pluss-inline-prev-next">.*?</div>\s*$', re.S)


def ordered(dirpath: pathlib.Path) -> list[str]:
    names = sorted({p.stem for p in dirpath.glob("*.md")}, key=str.lower)
    by_letter: dict[str, list[str]] = {}
    for n in names:
        by_letter.setdefault(n[0].upper(), []).append(n)
    return [n for letter in sorted(by_letter) for n in by_letter[letter]]


def fix_nav(t_name: str) -> None:
    t = BY_NAME[t_name]
    d = DOCS / "api" / t.dirname
    names = ordered(d)
    for i, n in enumerate(names):
        p = d / f"{n}.md"
        text = PREV_NEXT_RE.sub("", p.read_text(encoding="utf-8")).rstrip() + "\n"
        prev = names[i - 1] if i else None
        nxt = names[i + 1] if i < len(names) - 1 else None
        if prev or nxt:
            block = ['', '<div class="pluss-inline-prev-next">']
            if prev:
                block += [f'  <a class="prev-link" href="{prev}.html">', '    <span class="label">Previous</span>',
                          f'    <span class="title">{prev}</span>', '  </a>']
            if nxt:
                block += [f'  <a class="next-link" href="{nxt}.html">', '    <span class="label">Next</span>',
                          f'    <span class="title">{nxt}</span>', '  </a>']
            block.append('</div>')
            text += "\n".join(block) + "\n"
        p.write_text(text, encoding="utf-8")
    # landing toctree
    idx = index_for(t_name)
    s = idx.read_text(encoding="utf-8")
    m = re.search(r"(```\{toctree\}\n(?:.*?\n)*?)((?:api/%s/\S+\n)+)(```)" % re.escape(t.dirname), s)
    if not m:
        # accessor lines may sit between; fall back to a looser match
        m = re.search(r"(```\{toctree\}.*?\n\n(?:(?!api/%s/)\S.*\n)*)((?:api/%s/\S+\n)+)(```)" % (t.dirname, t.dirname), s, re.S)
    if not m:
        raise RuntimeError(f"toctree for {t_name} not found in {idx.name}")
    entries = "".join(f"api/{t.dirname}/{n}\n" for n in names)
    s = s[: m.start(2)] + entries + s[m.end(2):]
    idx.write_text(s, encoding="utf-8")


def refresh_sidebar_manifest() -> None:
    extracted: dict[str, list] = {}
    for class_name, cfg in g.CLASS_CONFIG.items():
        try:
            methods = g.extract_methods(class_name, cfg["header"])
        except (FileNotFoundError, RuntimeError):
            continue
        methods = [m for m in methods if m.name not in {"begin", "end"} or class_name in {"Catalog", "Registry"}]
        extracted[class_name] = methods
    try:
        extracted["DataFrameGrouped"] = g.extract_methods("DataFrameGrouped", ROOT / "PlussMic/src/table/DataFrame.h")
    except (FileNotFoundError, RuntimeError):
        extracted["DataFrameGrouped"] = []
    g.write_api_nav_manifest(extracted)


# ------------------------------------------------------------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--types", default=",".join(t.name for t in TYPES))
    ap.add_argument("--methods", default="")
    ap.add_argument("--no-nav", action="store_true")
    args = ap.parse_args()
    only = set(filter(None, args.methods.split(",")))
    written: list[tuple[str, str]] = []
    for t_name in args.types.split(","):
        t = BY_NAME[t_name]
        names = [m for m in methods_for(t_name) if not only or m in only]
        jobs = []
        specs = {}
        for name in names:
            ps = spec.page_spec(t, name)
            specs[name] = ps
            for i, ex in enumerate(ps["examples"]):
                jobs.append((name, i, ex))
        print(f"{t_name}: {len(names)} pages, {len(jobs)} examples -> compiling", flush=True)
        results = run_type(t_name, jobs)
        for name in names:
            ps = specs[name]
            outs = {i: results.get((name, i), "") for i in range(len(ps["examples"]))}
            bad = [i for i, o in outs.items() if o.startswith("EXCEPTION") or not o]
            if bad:
                print(f"  WARNING {t_name}::{name}: examples {bad} produced exception/empty output", flush=True)
            sigs, h = signature_block(t_name, name)
            write_page(t_name, name, ps, outs, sigs, h)
            written.append((t_name, name))
        if not args.no_nav:
            fix_nav(t_name)
    if not args.no_nav:
        refresh_sidebar_manifest()
    print(f"wrote {len(written)} pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
