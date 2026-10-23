#!/usr/bin/env python3
"""Compiles and runs the C++ examples of handwritten Series reference pages and writes the real output under each.

An example is a fenced ```cpp block followed by a `### Output` heading and a fenced ```text block:

    ```cpp
    auto x = pluss::make_series({1.0, 2.0});
    std::cout << x << "\\n";
    ```

    ### Output

    ```text
    ```

The snippet is wrapped in `int main() { ... }` after the standard headers, so it may use `std::cout` and `pluss::`
directly. The text block is replaced with the program's standard output (an uncaught exception fails the check).

Usage:
    python3 PlussQit/docs/scripts/verify_series_doc_examples.py PAGE.md [PAGE.md ...]            # rewrite outputs
    python3 PlussQit/docs/scripts/verify_series_doc_examples.py --check PAGE.md [PAGE.md ...]    # fail if stale
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
COMPILE = ROOT / "PlussTils" / "tools" / "verify_example.sh"
CACHE = Path(tempfile.gettempdir()) / "pluss_series_doc_cache"
BLOCK = re.compile(r"(```cpp\n)((?:(?!```).)*?)(```\n\n### Output\n\n```text\n)((?:(?!```).)*?)(```)", re.S)
HEADER = "#include <pluss/ma.h>\n\n#include <iomanip>\n#include <iostream>\n#include <vector>\n\n"
BANNED = re.compile(r"\b(matlab|numpy|pytorch|torch|python)\b", re.I)


def run_snippet(code: str) -> str:
    key = hashlib.sha256(code.encode()).hexdigest()
    CACHE.mkdir(exist_ok=True)
    cached = CACHE / key
    stamp = max(p.stat().st_mtime for p in (ROOT / "PlussQit" / "src" / "CodeM" / "core").glob("Series.*"))
    if cached.exists() and cached.stat().st_mtime > stamp:
        return cached.read_text()
    with tempfile.TemporaryDirectory() as d:
        src = Path(d) / "ex.cpp"
        exe = Path(d) / "ex"
        src.write_text(HEADER + "int main() {\n" + code + "\n  return 0;\n}\n")
        c = subprocess.run(["bash", str(COMPILE), str(src), str(exe)], capture_output=True, text=True)
        if c.returncode != 0:
            raise SystemExit("compile failed:\n" + code + "\n" + "\n".join(l for l in c.stderr.splitlines() if "error" in l))
        r = subprocess.run([str(exe)], capture_output=True, text=True)
        if r.returncode != 0:
            raise SystemExit("run failed:\n" + code + "\n" + r.stdout + r.stderr[-600:])
        out = r.stdout
    cached.write_text(out)
    return out


def process(path: Path, check: bool) -> bool:
    text = path.read_text()
    if BANNED.search(re.sub(r"```.*?```", "", text, flags=re.S)):
        print(f"{path}: prose names a banned term")
    stale = []

    def sub(m):
        out = run_snippet(m.group(2))
        if not out.endswith("\n"):
            out += "\n"
        if out != m.group(4):
            stale.append(1)
        return m.group(1) + m.group(2) + m.group(3) + out + m.group(5)

    new = BLOCK.sub(sub, text)
    n = len(BLOCK.findall(text))
    if check:
        print(f"{'STALE' if stale else 'ok   '} {path} ({n} examples)")
        return not stale
    if new != text:
        path.write_text(new)
    print(f"{path}: {n} examples, {len(stale)} updated")
    return True


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("pages", nargs="+")
    a = ap.parse_args()
    ok = all([process(Path(p), a.check) for p in a.pages])
    sys.exit(0 if ok else 1)
