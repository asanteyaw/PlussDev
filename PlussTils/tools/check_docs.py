"""Checks that every printed example in the PlussBI guide and API reference still prints what its page shows.

Each page holds a C++ block followed by an Output block. This script compiles each distinct program, runs it,
and compares its standard output with the page. A block whose output begins with "(no captured output" opens a
window or a browser, so it is compiled and linked but not run. Exit status is nonzero when any block fails.

Usage: python3 tools/check_docs.py [--jobs N]
"""
import concurrent.futures as cf
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PREFIXES = ("bi", "tils", "core", "loader", "loaders", "view")
MANUAL = ROOT / "docs/sphinx/manual"
API = ROOT / "docs/sphinx/api"
PAGES = sorted(
    [p for p in MANUAL.glob("*.md") if p.name.startswith(PREFIXES)]
    + [p for p in API.glob("*.md") if p.name.startswith(PREFIXES)]
    + [m for d in API.iterdir() if d.is_dir() and d.name.startswith(PREFIXES) for m in d.rglob("*.md")]
)
PAGES = sorted(set(PAGES))
BLOCK = re.compile(r"```cpp\n((?:(?!```)[\s\S])*?)\n```\n\nOutput\n\n```text\n((?:(?!```)[\s\S])*?)\n```")
NO_OUTPUT = "(no captured output"
VERIFY = ROOT / "tools/verify_example.sh"

def blocks():
    seen = {}
    for page in PAGES:
        for source, expected in BLOCK.findall(page.read_text()):
            key = hashlib.sha256(source.encode()).hexdigest()[:16]
            seen.setdefault(key, {"source": source, "expected": expected, "pages": []})["pages"].append(page.name)
    return seen

def check(item, key, workdir):
    src = workdir / f"{key}.cpp"
    exe = workdir / f"{key}.exe"
    src.write_text(item["source"] + "\n")
    build = subprocess.run(["bash", str(VERIFY), str(src), str(exe)], capture_output=True, text=True)
    if build.returncode != 0 or not exe.exists():
        return key, "COMPILE FAIL", build.stderr[-400:]
    if item["expected"].startswith(NO_OUTPUT):
        return key, "OK (compiled, not run)", ""
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    if run.returncode != 0:
        return key, f"EXIT {run.returncode}", run.stderr[-400:]
    got = [line.rstrip() for line in run.stdout.rstrip("\n").split("\n")]
    want = [line.rstrip() for line in item["expected"].rstrip("\n").split("\n")]
    if got != want:
        return key, "OUTPUT DIFFERS", "\n".join(["expected:"] + want + ["got:"] + got)
    return key, "OK", ""

def main():
    jobs = 6
    if "--jobs" in sys.argv:
        jobs = int(sys.argv[sys.argv.index("--jobs") + 1])
    items = blocks()
    failures = []
    with tempfile.TemporaryDirectory() as tmp, cf.ThreadPoolExecutor(max_workers=jobs) as pool:
        work = pathlib.Path(tmp)
        futures = [pool.submit(check, item, key, work) for key, item in items.items()]
        passed = 0
        for future in cf.as_completed(futures):
            key, status, detail = future.result()
            pages = ", ".join(sorted(set(items[key]["pages"])))
            if status.startswith("OK"):
                passed += 1
            else:
                failures.append((status, pages, detail))
    print(f"pages: {len(PAGES)}  distinct programs: {len(items)}  passed: {passed}  failed: {len(failures)}")
    for status, pages, detail in failures:
        print(f"\n[{status}] {pages}\n{detail}")
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
