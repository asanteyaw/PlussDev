#!/usr/bin/env python3
"""Fail when published docs leak library-owned C++ type definitions or kind words.

Public docs teach options with tables (field | type | default | meaning) and values with
value tables, and show flat qualified signatures. They never show `struct`, `enum class` or
`class` definitions of library types and never describe a library type as "a struct",
"an enum class", "the options struct" and so on.

Checks (outside the allowlist):
  1. type-body: a fenced code block line that defines a type
     (`struct X {`, `enum class X ...`, `class X : public Base {`, ...);
  2. prose: the words struct/structs, enum/enum class, "scoped enumeration" / "an enumeration",
     and C++ kind phrases such as "options class" or "base class", outside code fences.

User-defined helper types that an example program needs (a local `struct Row`, a user
class deriving from a library base) are legitimate; list them in
tools/docs_lint_types_allowlist.txt with a reason.

Usage:
  python3 tools/docs_lint_types.py --check          # exit 1 on any violation
  python3 tools/docs_lint_types.py --list           # print violations and exit 0
  python3 tools/docs_lint_types.py --check DIR ...  # restrict to directories (relative to repo root)
"""
import fnmatch
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT = ["PlussQit/docs", "PlussMic/docs", "PlussMa/docs", "PlussAI/docs", "PlussTils/docs",
           "PlussPl/docs", "docs/sphinx"]
SKIP_PARTS = {"_build", "_local", "node_modules", ".doctrees"}
# Names the site sync never publishes (see plussible/scripts/sync-docs.py).
INTERNAL_NAMES = {
    "README.md", "build_outputs.md", "plussmic_docs_hub.md", "Series_Doc_Authoring_Standard.md",
    "API_Doc_Authoring_Standard.md", "CodeM_MethodRoutingPlan.md", "derivative_coverage.md",
    "PATCHES.md", "USER_GUIDE_STANDARD.md", "dl_torch_maturity_blueprint.md",
    "autodiff_architecture.md", "optimization_diff_contract.md", "plusspl_blueprint.md",
    "dataframe_benchmarks.md",
}
FENCE = re.compile(r"^\s*(?::\s*)?```")  # also matches definition-list fences: ": ```cpp"
DIRECTIVE = re.compile(r"^\s*(?::\s*)?```+\s*\{(?!code-block|code|sourcecode)")  # MyST directive: body is prose
ALLOWLIST = Path(__file__).with_name("docs_lint_types_allowlist.txt")

DEF = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?(?:(?:struct|class|enum\s+class|enum\s+struct|enum|union))\s+"
    r"(?P<name>[A-Za-z_][\w:]*)\s*(?P<rest>[^;]*)$")
DEF_ONELINE = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?(?:struct|class|enum\s+class|enum|union)\s+(?P<name>[A-Za-z_][\w:]*)[^;]*\{.*\}\s*;")
PROSE = [
    (re.compile(r"\bstructs?\b", re.I), "struct"),
    (re.compile(r"\benum(?:s| class(?:es)?)?\b", re.I), "enum"),
    (re.compile(r"\b(?:scoped|an?|the|this|that) enumeration(?: type)?\b(?! of the)", re.I), "enumeration"),
    (re.compile(r"\benumeration type\b", re.I), "enumeration type"),
    (re.compile(r"\b(?:options|option|result|abstract|base|final) class(?:es)?\b", re.I), "class kind"),
    (re.compile(r"\bclass (?:type|template|definition|hierarchy)\b", re.I), "class kind"),
]
# Plain mathematical/combinatorial "enumeration" and ML "class" are not leaks.
MATH_ENUM = re.compile(r"\b(?:the|a|an) (?:positional |full |exhaustive |combinatorial |lexicographic )?enumeration "
                       r"(?:produces|order|is|of the|of all|operation|that)\b", re.I)


def load_allowlist():
    entries = []
    if not ALLOWLIST.exists():
        return entries
    for raw in ALLOWLIST.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = [x.strip() for x in line.split("|")]
        if len(parts) != 4 or not parts[3]:
            raise SystemExit(f"bad allowlist line (need kind | path-glob | name-glob | reason): {raw}")
        entries.append(tuple(parts))
    return entries


def allowed(entries, kind, rel, name):
    for k, path_glob, name_glob, _ in entries:
        if k == kind and fnmatch.fnmatch(rel, path_glob) and fnmatch.fnmatch(name, name_glob):
            return True
    return False


def scan_file(path, rel, entries):
    out = []
    in_fence = False  # True only inside a code fence; ```{note}-style directive bodies are prose
    in_directive = False
    for n, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        if FENCE.match(line):
            if in_fence or in_directive:
                in_fence = in_directive = False
            elif DIRECTIVE.match(line):
                in_directive = True
                line = line.split("}", 1)[1]  # the directive title is prose too
                for rx, label in PROSE:
                    m = rx.search(line)
                    if m and not allowed(entries, "prose", rel, m.group(0).lower()):
                        out.append((n, "prose:" + label, line.strip()[:140]))
                        break
            else:
                in_fence = True
            continue
        if in_fence:
            m = DEF_ONELINE.match(line) or DEF.match(line)
            if m:
                name = m.group("name")
                if not allowed(entries, "type", rel, name):
                    out.append((n, "type-body", line.strip()[:100]))
            continue
        for rx, label in PROSE:
            m = rx.search(line)
            if not m:
                continue
            if label == "enumeration" and MATH_ENUM.search(line):
                continue
            if allowed(entries, "prose", rel, m.group(0).lower()):
                continue
            out.append((n, "prose:" + label, line.strip()[:140]))
            break
    return out


def main(argv):
    mode = "check"
    dirs = []
    for a in argv:
        if a == "--list":
            mode = "list"
        elif a == "--check":
            mode = "check"
        else:
            dirs.append(a)
    entries = load_allowlist()
    bad = 0
    for d in dirs or DEFAULT:
        for path in sorted((ROOT / d).rglob("*.md")) + sorted((ROOT / d).rglob("*.md.in")):
            parts = set(path.relative_to(ROOT).parts)
            if SKIP_PARTS & parts or path.name in INTERNAL_NAMES or path.is_symlink():
                continue
            rel = path.relative_to(ROOT).as_posix()
            for n, kind, text in scan_file(path, rel, entries):
                bad += 1
                print(f"{rel}:{n}: {kind}: {text}")
    print(f"{bad} type-leak hit(s)")
    return 1 if (bad and mode == "check") else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
