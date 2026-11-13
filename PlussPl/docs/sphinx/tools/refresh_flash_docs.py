#!/usr/bin/env python3
"""Refreshes the printed output and the figures of the handwritten flash reference pages from their own example code.

For every `Example N.` of a page in api/flash/ the tool compiles and runs the example (the code is the body of `main()` in
the standard harness of plotdocs.py), then

  * rewrites the ```text block that follows the code with the real standard output (adds one if the example prints
    something and has none; reports an output block the example no longer prints),
  * renders each `X.show();` of the code as `X.save_svg(...)` and copies the figures over the `![...](img/...)`
    files the page shows, in order.

The prose is left as it is: the tool only makes the output and the pictures true. Examples that fail to build or run are
listed at the end (nothing of them is changed), so they can be fixed in the page.

Usage:
    python3 tools/refresh_flash_docs.py [page ...]       # rewrite (default: all pages)
    python3 tools/refresh_flash_docs.py --check [page ...]  # report what is stale, change nothing
Environment: PLOTDOCS_CACHE (a fresh directory after any library change), SDKROOT.
"""
import argparse, concurrent.futures, hashlib, os, pathlib, re, shutil, subprocess, sys, tempfile

HERE = pathlib.Path(__file__).resolve().parent
SPHINX = HERE.parent
ROOT = SPHINX.parents[2]
API = SPHINX / "api" / "flash"
IMG = API / "img"
VERIFY = ROOT / "PlussTils" / "tools" / "verify_example.sh"
CACHE = pathlib.Path(os.environ.get("PLOTDOCS_CACHE", tempfile.gettempdir())) / "flash_refresh_cache"

HARNESS = """#include <pluss/pl/plusspl.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace pluss::pl;
int main() {
%s
}
"""
HEADING = re.compile(r"^Example (\d+)[^\n]*\n", re.M)
SHOW = re.compile(r"(\b[A-Za-z_][A-Za-z_0-9]*)\.show\(\)\s*;")
FENCE = re.compile(r"```(\w*)\n(.*?)```", re.S)
IMAGE = re.compile(r"!\[[^\]]*\]\((img/[^)]+\.svg)\)")


class Example:
    def __init__(self, page, number, start, end, text):
        self.page, self.number, self.start, self.end, self.text = page, number, start, end, text


def split_examples(text):
    """The (start, end) spans of the examples of a page: from one `Example N.` heading to the next heading of any kind."""
    spans = []
    marks = [m for m in HEADING.finditer(text)]
    stop = re.compile(r"^## ", re.M)
    for i, m in enumerate(marks):
        end = marks[i + 1].start() if i + 1 < len(marks) else len(text)
        tail = stop.search(text, m.end())
        if tail and tail.start() < end:
            end = tail.start()
        spans.append((int(m.group(1)), m.start(), end))
    return spans


def compile_and_run(page, number, code, show_targets):
    """Run one example. `show_targets` is the list of svg paths the successive `.show()` calls are to write."""
    shows = iter(show_targets)

    def replace(m):
        target = next(shows, None)
        return f'{m.group(1)}.save_svg("{target}");' if target else ""
    body = SHOW.sub(replace, code)
    text = HARNESS % body
    if re.search(r"pluss::table|DataFrame|Slate|\.frame\(|Dataset", code):
        text = "#include <pluss/table/table.h>\n" + text
    salt = "".join(hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in os.environ.get("PLUSSDEV_EXTRA_SRCS", "").split() if pathlib.Path(p).exists())
    key = hashlib.sha256((text.replace(str(pathlib.Path(show_targets[0]).parent) if show_targets else "", "<out>") + salt).encode()).hexdigest()[:20]
    hit = CACHE / key
    if (hit / "stdout.txt").exists():
        for i, target in enumerate(show_targets):
            cached = hit / f"figure{i}.svg"
            if cached.exists():
                shutil.copy(cached, target)
        return (hit / "stdout.txt").read_text(), None
    work = pathlib.Path(tempfile.mkdtemp(prefix="flashdocs_"))
    try:
        cpp, exe = work / "e.cpp", work / "e.exe"
        cpp.write_text(text)
        env = dict(os.environ)
        env.setdefault("SDKROOT", subprocess.run(["xcrun", "--show-sdk-path"], capture_output=True, text=True).stdout.strip())
        b = subprocess.run(["bash", str(VERIFY), str(cpp), str(exe)], capture_output=True, text=True, env=env)
        if b.returncode != 0:
            return None, "build failed: " + " | ".join(l.strip() for l in b.stderr.splitlines() if "error" in l)[:400]
        try:
            r = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300, env=env, cwd=work)
        except subprocess.TimeoutExpired:
            return None, "run timed out after 300 s"
        if r.returncode != 0:
            return None, "run failed: " + (r.stderr.strip().splitlines() or ["?"])[0][:300]
        hit.mkdir(parents=True, exist_ok=True)
        (hit / "stdout.txt").write_text(r.stdout)
        for i, target in enumerate(show_targets):
            if pathlib.Path(target).exists():
                shutil.copy(target, hit / f"figure{i}.svg")
        return r.stdout, None
    finally:
        shutil.rmtree(work, ignore_errors=True)


def process_page(path, check):
    text = path.read_text()
    spans = split_examples(text)
    if not spans:
        return path.stem, text, [], []
    notes, failures = [], []
    pieces, cursor = [], 0
    out_dir = pathlib.Path(tempfile.mkdtemp(prefix="flashfig_"))
    try:
        for number, start, end in spans:
            segment = text[start:end]
            fences = list(FENCE.finditer(segment))
            code_fence = next((f for f in fences if f.group(1) == "cpp"), None)
            if not code_fence:
                continue
            code = code_fence.group(2)
            if re.search(r"flash::animate[34]\(", code) and "<window-launch" in segment:
                continue  # opens a window: cannot run headless, its companion render example is checked on its own
            images = IMAGE.findall(segment)
            shows = SHOW.findall(code)
            # Figures the page shows beyond the show() calls come from the other studios of the example, in order.
            extra_vars = [v for v in re.findall(r"auto\s+(\w+)\s*=\s*studio\(", code) if v not in shows]
            extra = extra_vars[:max(0, len(images) - len(shows))]
            targets = [str(out_dir / f"{path.stem}-{number}-{k}.svg") for k in range(len(shows) + len(extra))]
            run_code = code + "".join(f'\n{v}.save_svg("{targets[len(shows) + i]}");' for i, v in enumerate(extra))
            stdout, error = compile_and_run(path.stem, number, run_code, targets)
            if error:
                failures.append((path.stem, number, error))
                continue
            new = segment
            text_fence = next((f for f in fences if f.group(1) == "text" and f.start() > code_fence.end()), None)
            printed = stdout.rstrip("\n")
            if text_fence:
                old = text_fence.group(2).rstrip("\n")
                if old != printed:
                    notes.append(f"{path.stem} example {number}: output " + ("changed" if printed else "no longer printed"))
                    if printed:
                        new = new[:text_fence.start(2)] + printed + "\n" + new[text_fence.end(2):]
                    else:
                        before = new[:text_fence.start()].rstrip("\n")
                        # drop the block and the label line that introduced it
                        before = re.sub(r"\n+(### Output|Output:)$", "", before)
                        new = before + "\n\n" + new[text_fence.end():].lstrip("\n")
            elif printed:
                notes.append(f"{path.stem} example {number}: output block added")
                insert = "\n\nOutput:\n\n```text\n" + printed + "\n```"
                new = new[:code_fence.end()] + insert + new[code_fence.end():]
            for k, image in enumerate(images):
                if k < len(targets) and pathlib.Path(targets[k]).exists():
                    dest = API / image
                    fresh = pathlib.Path(targets[k]).read_bytes()
                    if not dest.exists() or dest.read_bytes() != fresh:
                        notes.append(f"{path.stem} example {number}: figure {image} " + ("new" if not dest.exists() else "changed"))
                        if not check:
                            dest.parent.mkdir(parents=True, exist_ok=True)
                            dest.write_bytes(fresh)
                else:
                    failures.append((path.stem, number, f"figure {image} has no matching show() call"))
            pieces.append((start, end, new))
        out, cursor = [], 0
        for start, end, new in pieces:
            out.append(text[cursor:start])
            out.append(new)
            cursor = end
        out.append(text[cursor:])
        return path.stem, "".join(out), notes, failures
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pages", nargs="*")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--jobs", type=int, default=4)
    a = ap.parse_args()
    paths = sorted(API.glob("*.md")) if not a.pages else [API / (p if p.endswith(".md") else p + ".md") for p in a.pages]
    paths = [p for p in paths if p.name not in ("index.md",)]
    all_notes, all_failures = [], []
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        for stem, text, notes, failures in pool.map(lambda p: process_page(p, a.check), paths):
            all_notes += notes
            all_failures += failures
            path = API / f"{stem}.md"
            if not a.check and path.read_text() != text:
                path.write_text(text)
    for n in all_notes:
        print(n)
    for page, number, error in all_failures:
        print(f"FAILED {page} example {number}: {error}")
    print(f"{len(paths)} pages, {len(all_notes)} changes, {len(all_failures)} failures")
    if a.check and all_notes:
        sys.exit(1)


if __name__ == "__main__":
    main()
