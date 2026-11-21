#!/usr/bin/env python3
"""Re-run the examples of existing API pages and report which printed outputs no longer match.

Used after the NULL / NaN / infinity separation to find pages whose recorded output was produced under the old rule (NaN as a
missing cell). Every `cpp` block that is followed by a `### Output` block is wrapped in main(), compiled, run, and compared with
the recorded output. Nothing is rewritten unless --write is given, and then only the Output blocks of examples that compiled.

Usage: rerun_pages.py PAGE.md ...   [--write] [--json out.json]
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
VERIFY = ROOT / "PlussTils/tools/verify_example.sh"
OUT_BLOCK = re.compile(r"(\s*### Output[^\n]*\n\n```text\n)(.*?)(```\n)", re.S)
OUT_BLOCK = re.compile(r"(\s*### Output[^\n]*\n\n```text\n)((?:(?!```).)*?)(```\n)", re.S)
PAT = re.compile(r"(```cpp\n)((?:(?!```).)*?)(```\n\n### Output[^\n]*\n\n```text\n)(.*?)(```\n)", re.S)
HEAD = """#include <pluss/pluss.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
using namespace pluss::table;
using namespace pluss::kindle;
using namespace pluss::scribble;
using namespace pluss::kindle::literals;
int main() {
"""


HEAD_BASE = HEAD
NEXUS_USES = "using namespace pluss::bundle;\nusing namespace pluss::bundle::nexus;\nnamespace nx = pluss::bundle::nexus;\n"


NX_FIXTURE = """
auto nx = [] {
  using namespace pluss::table;
  auto customers = DataFrame({{"customer_id", Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
                              {"region_id", Shelf(pluss::make_series({10.0, 20.0, 10.0}))}});
  auto regions = DataFrame({{"region_id", Shelf(pluss::make_series({10.0, 20.0}))},
                            {"zone", Shelf(pluss::make_note({"west", "east"}))}});
  auto events = DataFrame({{"event_id", Shelf(pluss::make_series({101.0, 102.0, 103.0, 104.0, 105.0}))},
                           {"customer_id", Shelf(pluss::make_series({1.0, 1.0, 2.0, 2.0, 2.0}))},
                           {"spend", Shelf(pluss::make_series({5.0, 10.0, 2.0, 4.0, 8.0}))},
                           {"at", Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-05T00:00:00Z", "2024-01-09T00:00:00Z",
                                                              "2024-01-12T00:00:00Z", "2024-01-18T00:00:00Z"}))}});
  pluss::bundle::nexus::TableMeta customers_meta;
  customers_meta.primary_key = {"customer_id"};
  pluss::bundle::nexus::TableMeta events_meta;
  events_meta.time_field = "at";
  pluss::bundle::nexus::Relationship to_region;
  to_region.name = "customers_to_regions";
  to_region.left_table = "customers";
  to_region.right_table = "regions";
  to_region.keys = {{"region_id", "region_id"}};
  to_region.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;
  pluss::bundle::nexus::Relationship to_customer;
  to_customer.name = "events_to_customers";
  to_customer.left_table = "events";
  to_customer.right_table = "customers";
  to_customer.keys = {{"customer_id", "customer_id"}};
  to_customer.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;
  return pluss::bundle::Nexus({{"customers", customers}, {"regions", regions}, {"events", events}})
      .set_table_meta("customers", customers_meta)
      .set_table_meta("events", events_meta)
      .add_relationship(to_region)
      .add_relationship(to_customer);
}();
"""


def compile_run(code: str, work: Path, name: str) -> tuple[str, str]:
    src = work / f"{name}.cpp"
    exe = work / name
    for variant in (code, code.replace("using namespace pluss::kindle;\nusing namespace pluss::scribble;\n", "")):
        src.write_text(variant)
        r = subprocess.run([str(VERIFY), str(src), str(exe)], capture_output=True, text=True)
        if r.returncode == 0 and exe.exists():
            run = subprocess.run([str(exe)], capture_output=True, text=True, cwd=work)
            if run.returncode != 0:
                return "RUNFAIL", run.stderr[-300:] + run.stdout[-200:]
            return "OK", run.stdout
    return "COMPILE", r.stderr[-300:] + r.stdout[-300:]


def run_page(bodies: list[str], work: Path) -> list[tuple[str, str]]:
    """Examples on one page often share variables, so first try them as one program (a shared scope); if the page redefines a name,
    fall back to one program per example."""
    marker = "@@EX"
    joined = HEAD + "\n".join(f'std::cout << "\\n{marker} {i}\\n";\n{b}' for i, b in enumerate(bodies)) + "\n}\n"
    status, out = compile_run(joined, work, "page")
    if status == "OK":
        parts = re.split(r"\n" + marker + r" (\d+)\n", "\n" + out)
        got = {int(parts[i]): parts[i + 1] for i in range(1, len(parts) - 1, 2)}
        return [("OK", got.get(i, "")) for i in range(len(bodies))]
    return [compile_run(HEAD + b + "\n}\n", work, f"e{i}") for i, b in enumerate(bodies)]


SYNTAX = Path(__file__).resolve().parent / "syntax_check.sh"


def run_page_inc(bodies: list[str], work: Path) -> list[tuple[str, str]]:
    """Add the examples one at a time to a shared scope (a failing one is retried in its own braces, then given up on), so a page whose
    examples share variables, or redefine one, still gets every example that can compile run."""
    marker = "@@EX"
    accepted: list[str] = []
    verdict: list[tuple[str, str]] = [("COMPILE", "")] * len(bodies)
    for i, b in enumerate(bodies):
        variants = [b]
        tried_rename = False
        v = 0
        while v < len(variants) + 1:
            variant = variants[v] if v < len(variants) else "{\n" + b + "\n}"
            piece = f'std::cout << "\\n{marker} {i}\\n";\n{variant}'
            prog = HEAD + "\n".join(accepted + [piece]) + "\n}\n"
            src = work / "inc.cpp"
            src.write_text(prog)
            r = subprocess.run([str(SYNTAX), str(src), "x"], capture_output=True, text=True)
            if r.returncode == 0:
                accepted.append(piece)
                verdict[i] = ("OK", "")
                break
            verdict[i] = ("COMPILE", r.stderr[-400:])
            if not tried_rename:
                names = set(re.findall(r"redefinition of '(\w+)'", r.stderr))
                if names:
                    tried_rename = True
                    renamed = b
                    for nm in names:
                        renamed = re.sub(r"\b%s\b" % re.escape(nm), f"{nm}_ex{i + 1}", renamed)
                    variants.append(renamed)
            v += 1
    if accepted:
        status, out = compile_run(HEAD + "\n".join(accepted) + "\n}\n", work, "incpage")
        if status == "OK":
            parts = re.split(r"\n" + marker + r" (\d+)\n", "\n" + out)
            got = {int(parts[k]): parts[k + 1] for k in range(1, len(parts) - 1, 2)}
            return [("OK", got.get(i, "")) if verdict[i][0] == "OK" else verdict[i] for i in range(len(bodies))]
        return [(status, out) for _ in bodies]
    return verdict


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pages", nargs="+")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--incremental", action="store_true")
    ap.add_argument("--json")
    args = ap.parse_args()
    report = {}
    for page in args.pages:
        p = Path(page)
        text = p.read_text(encoding="utf-8")
        start = text.index("## Examples") if "## Examples" in text else 0
        head, body = text[:start], text[start:]
        results = []
        new_body = body
        with tempfile.TemporaryDirectory(prefix="rerun_") as d:
            work = Path(d)
            pieces = []
            last = 0
            matches = list(PAT.finditer(body))
            globals()["HEAD"] = (HEAD_BASE.replace("int main() {\n", NEXUS_USES + "int main() {\n" + (NX_FIXTURE if (("/nexus-pull/" in page or "/nexus-query/" in page) or "/nexus-query/" in page) else ""))) if "/nexus" in page else HEAD_BASE
            # "Given `orders.csv`:" followed by a text block: the page shows the input file, so write it where the examples run.
            for fm in re.finditer(r"`([\w./-]+\.(?:csv|json|txt|tsv))`[^\n]*:\n\n```[a-z]*\n(.*?)```", body, re.S):
                (work / fm.group(1)).parent.mkdir(parents=True, exist_ok=True)
                (work / fm.group(1)).write_text(fm.group(2))
            runs = (run_page_inc if args.incremental else run_page)([m.group(2) for m in matches], work) if matches else []
            for i, (m, (status, got)) in enumerate(zip(matches, runs), start=1):
                expected = m.group(4)
                norm = lambda t: [ln.rstrip() for ln in t.strip().splitlines() if ln.strip()]  # blank lines and trailing spaces do not count
                same = status == "OK" and norm(got) == norm(expected)
                results.append({"n": i, "status": "same" if same else ("CHANGED" if status == "OK" else status), "got": "" if same else got[:800]})
                following = []  # further Output blocks that belong to the same code block
                pos = m.end()
                while True:
                    mm = OUT_BLOCK.match(body, pos)
                    if not mm:
                        break
                    following.append(mm)
                    pos = mm.end()
                multi = bool(following)
                if multi and not same:
                    # one code block, several Output blocks: give each its own slice of the program output when the line counts agree
                    blocks = [m.group(4)] + [f.group(2) for f in following]
                    nb = lambda t: [ln for ln in t.splitlines() if ln.strip()]
                    glued = re.sub(r"(feat\])(?=[^\n])", "\\1\n", got)
                    lines = nb(glued)
                    counts = [len(nb(b)) for b in blocks]
                    if status == "OK" and sum(counts) == len(lines):
                        same = False
                        results[-1]["status"] = "split"
                        pieces.append(body[last:m.start()])
                        if args.write and "Traceback" not in got:
                            k = 0
                            slices = []
                            for c in counts:
                                slices.append("\n".join(lines[k:k + c]) + "\n")
                                k += c
                            out_txt = m.group(1) + m.group(2) + m.group(3) + slices[0] + m.group(5)
                            for f, sl in zip(following, slices[1:]):
                                out_txt += f.group(0)[: f.start(2) - f.start(0)] + sl + f.group(3)
                            pieces.append(out_txt)
                        else:
                            pieces.append(body[m.start():following[-1].end()])
                        last = following[-1].end()
                        continue
                    results[-1]["status"] = "MULTI-OUTPUT (review by hand)"
                if args.write and status == "OK" and not same and not multi and "Traceback" not in got:
                    pieces.append(body[last:m.start()])
                    pieces.append(m.group(1) + m.group(2) + m.group(3) + got.strip("\n").rstrip() + "\n" + m.group(5))
                else:
                    pieces.append(body[last:m.start()])
                    pieces.append(m.group(0))
                last = m.end()
            pieces.append(body[last:])
            new_body = "".join(pieces)
        if args.write and new_body != body:
            p.write_text(head + new_body, encoding="utf-8")
        report[page] = results
        flag = [r for r in results if r["status"] != "same"]
        print(f"{page}: {len(results)} examples, {len(flag)} differ" + ("" if not flag else "  " + ", ".join(f"#{r['n']}:{r['status']}" for r in flag)), flush=True)
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
