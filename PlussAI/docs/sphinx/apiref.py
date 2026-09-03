#!/usr/bin/env python3
"""Scaffold, wire, sig-guard, and lint the handwritten PlussAI / Lymph API reference.

The pages are hand-authored; this tool owns only *structure* and *discipline*.
It creates stubs, wires the persistent sidebar toctrees and prev/next blocks,
guards signatures against header drift, and lints every page against the
standing contracts:

    docs/lymph_api_doc_standards.md
    docs/lymph_api_reference_doc_standards.md

Layout follows the public granularity rhythm (family -> [subfamily ->] type):

    api/lymph/<family>.md                     family landing
    api/lymph/<family>/<Type>.md              type landing (directly under a family)
    api/lymph/<family>/<Type>/Introduction.md type role, plus any notes / contract
                                             / gotchas as trailing `## ...` sections
    api/lymph/<family>/<Type>/<method>.md     one page per public method (alphabetical)
    api/lymph/<family>/<subfamily>/<Type>...  same shape one level deeper
    api/lymph/<family>/<name>.md              free function
    api/lymph/<family>/<Type>Options.md       options object (fields taught in prose)

Subcommands
-----------
import    Seed apiref_manifest.toml from the existing tree (one-time per family).
scaffold  Create every missing landing / method / options page from the manifest.
wire      Rewrite the sidebar toctrees, the prev/next blocks, and apiref_nav.js.
sighash   Report or (--write) re-stamp <!-- pluss-signature-hash --> markers.
lint      Enforce the standards; nonzero exit on any problem.

Standard library only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

DOCS = Path(__file__).resolve().parent
API = DOCS / "api"
MANIFEST = DOCS / "apiref_manifest.toml"
REPO_ROOT = DOCS.parents[2]

TOC_START = "<!-- apiref:toctree:start -->"
TOC_END = "<!-- apiref:toctree:end -->"
NAV_START = "<!-- apiref:nav:start -->"
NAV_END = "<!-- apiref:nav:end -->"
HAND = "<!-- pluss-hand-authored -->"
SIGHASH_RE = re.compile(r"<!-- pluss-signature-hash:\s*([0-9a-f]{12}) -->")

# Terms that must never appear in a published page. Classroom framing and
# cross-library comparison both read wrong in an API reference; internal
# framework code names must not leak.
FORBIDDEN_TERMS = [
    r"\bteach(?:es|ing|er)?\b", r"\bscikit-?learn\b", r"\bsklearn\b",
    r"\bnumpy\b", r"\bpandas\b", r"\bpytorch\b", r"\btorch\b",
    r"\btensorflow\b", r"\bkeras\b", r"\bnodej\b", r"\bnodek\b", r"\bnodeb\b",
    r"pluss::msharp",
]

# Signature blocks must be public callable declarations, never pasted bodies.
SIG_BODY_MARKERS = ("class ", "struct ", "public:", "private:", "namespace ")

# "*" prefix = required. Order is canonical; extra sections may only trail.
SECTIONS: dict[str, list[str]] = {
    "method": ["*Signature", "Parameters", "Returns", "Errors",
               "*Practical Examples", "See Also"],
    "function": ["*Signature", "Parameters", "Returns", "Errors",
                 "*Practical Examples", "See Also"],
    # A class landing page hosts the ## Methods table + toctree; the object's
    # role prose lives on its Introduction child page, checked separately.
    "class": ["Signature", "Construction", "State", "Introduction", "*Methods", "*Changelog"],
    "options": ["*Fields", "Guidance", "See Also"],
    "family": [],
    "page": [],
}


# --------------------------------------------------------------------------- #
# Manifest
# --------------------------------------------------------------------------- #
@dataclass
class Symbol:
    qualname: str
    kind: str                      # class | function | options
    family: str
    page: str
    summary: str = ""
    since: str = "0.1"
    changed: list[str] = field(default_factory=list)
    methods: list[str] = field(default_factory=list)
    fields: list[str] = field(default_factory=list)
    overloads: int = 1
    toggles: list[str] = field(default_factory=list)
    guide: str = ""
    see_also: list[str] = field(default_factory=list)
    # An options symbol may name its owning class's qualname. It is then nested
    # under that object in the sidebar (and its toctree) instead of sitting as a
    # sibling of the object at family level.
    owner: str = ""

    @property
    def name(self) -> str:
        return self.qualname.split("::")[-1]

    @property
    def method_dir(self) -> Path:
        return DOCS / Path(self.page).with_suffix("")

    def method_page(self, method: str) -> Path:
        return self.method_dir / f"{method}.md"

    @property
    def min_examples(self) -> int:
        return max(1, self.overloads) + len(self.toggles)


@dataclass
class Family:
    path: str
    page: str
    title: str
    children: list[str] = field(default_factory=list)   # verbatim toctree refs
    maxdepth: int = 2


@dataclass
class Manifest:
    framework: str
    fw_title: str
    fw_summary: str
    fw_include: str
    fw_header: str
    families: list[Family]
    symbols: list[Symbol]


def load_manifest() -> Manifest:
    if not MANIFEST.exists():
        sys.exit(f"no manifest at {MANIFEST.name}; run `apiref.py import --family <f>` first")
    data = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    fw_key, fw = next(iter(data["framework"].items()))
    return Manifest(
        framework=fw_key,
        fw_title=fw.get("title", fw_key),
        fw_summary=fw.get("summary", ""),
        fw_include=fw.get("include", ""),
        fw_header=fw.get("header", ""),
        families=[Family(**f) for f in data.get("family", [])],
        symbols=[Symbol(**s) for s in data.get("symbol", [])],
    )


# --- tiny TOML emitter (flat schema: str | int | list[str]) ----------------- #
def _v(x) -> str:
    if isinstance(x, list):
        return "[" + ", ".join(_v(i) for i in x) + "]"
    if isinstance(x, int):
        return str(x)
    return '"' + str(x).replace("\\", "\\\\").replace('"', '\\"') + '"'


def _tbl(rows: dict) -> str:
    return "".join(f"{k} = {_v(val)}\n" for k, val in rows.items() if val not in ("", [], None))


def dump_manifest(m: Manifest) -> str:
    out = [
        "# apiref_manifest.toml -- generated by `apiref.py import`, hand-edit freely.",
        "# `apiref.py lint` keeps this file, the pages, and the public header in sync.",
        "",
        f"[framework.{m.framework}]",
        _tbl({"title": m.fw_title, "summary": m.fw_summary,
              "include": m.fw_include, "header": m.fw_header}),
    ]
    for fam in m.families:
        out += ["[[family]]", _tbl({"path": fam.path, "page": fam.page, "title": fam.title,
                                    "maxdepth": fam.maxdepth, "children": fam.children})]
    for s in m.symbols:
        out += ["[[symbol]]", _tbl({
            "qualname": s.qualname, "kind": s.kind, "family": s.family, "page": s.page,
            "summary": s.summary, "since": s.since, "changed": s.changed,
            "methods": s.methods, "fields": s.fields,
            "overloads": s.overloads if s.kind == "function" else "",
            "toggles": s.toggles, "guide": s.guide, "see_also": s.see_also,
        })]
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------- #
# Page text helpers
# --------------------------------------------------------------------------- #
H1 = re.compile(r"^#\s+(.*)$", re.M)
H2 = re.compile(r"^##\s+(.+?)\s*$", re.M)
H3 = re.compile(r"^###\s+(.+?)\s*$", re.M)
LITINC = re.compile(r"^```\{literalinclude\}", re.M)
TOCBLOCK = re.compile(r"```\{toctree\}\n(.*?)```", re.S)
CPP_PARAM = re.compile(r"[(,]\s*(?:const\s+)?[\w:]+(?:\s*<[^>]*>)?(?:\s*[&*])?\s+(\w+)\s*(?==|,|\))")


def title_of(text: str) -> str:
    m = H1.search(text)
    return m.group(1).strip().strip("`") if m else ""


def h2s(text: str) -> list[str]:
    return [h.strip() for h in H2.findall(text)]


def lede(text: str) -> str:
    body = re.sub(r"^\s*---.*?---\s*", "", text, flags=re.S)
    body = H1.sub("", body, count=1).strip()
    para = body.split("\n\n", 1)[0].strip()
    return "" if para.startswith(("#", "[Back")) else " ".join(para.split())


def toctree_refs(text: str) -> list[str]:
    m = TOCBLOCK.search(text)
    if not m:
        return []
    refs = []
    for line in m.group(1).splitlines():
        line = line.strip()
        if line and not line.startswith(":"):
            refs.append(line.split("<")[-1].rstrip(">").strip() if "<" in line else line)
    return refs


def sig_params(text: str) -> list[str]:
    """Parameter names of the primary declarations in ## Signature.

    Only lines up to the first `//` comment count: a Signature block often
    pastes related factory/helper declarations after a comment for context,
    and their parameter names are not part of this symbol's contract.
    """
    m = re.search(r"##\s+Signature\s*\n+```cpp\n(.*?)```", text, re.S)
    if not m:
        return []
    body = m.group(1).split("\n//", 1)[0]
    seen: list[str] = []
    for p in CPP_PARAM.findall(body):
        if p not in seen and p not in ("options", "value"):
            seen.append(p)
    return seen


def normalized_sig(text: str) -> str:
    """Signature text with names/comments stripped -> stable across renames."""
    m = re.search(r"##\s+Signature\s*\n+```cpp\n(.*?)```", text, re.S)
    if not m:
        return ""
    body = re.sub(r"//.*", "", m.group(1))
    body = re.sub(r"\b(\w+)\s*(?=[,)])", "", body)   # drop trailing identifiers
    return re.sub(r"\s+", " ", body).strip()


def sig_hash(text: str) -> str:
    return hashlib.sha256(normalized_sig(text).encode()).hexdigest()[:12]


def read_header_surface(m: Manifest) -> str | None:
    """`header` may point at a single file or a directory of headers."""
    if not m.fw_header:
        return None
    target = REPO_ROOT / m.fw_header
    if target.is_dir():
        return "\n".join(p.read_text(encoding="utf-8", errors="ignore")
                         for p in sorted(target.rglob("*.h")))
    if target.is_file():
        return target.read_text(encoding="utf-8", errors="ignore")
    return None


# --------------------------------------------------------------------------- #
# import
# --------------------------------------------------------------------------- #
NON_METHOD = {"Introduction", "Notes", "Overview"}


def cmd_import(args: argparse.Namespace) -> int:
    fw = args.family
    root = API / fw
    if not root.exists():
        sys.exit(f"no such framework tree: {root}")

    all_md = sorted(root.rglob("*.md"))
    type_stems = {p.stem for p in all_md if p.stem[:1].isupper() and p.stem not in NON_METHOD}

    families: list[Family] = []
    symbols: list[Symbol] = []
    for md in all_md:
        text = md.read_text(encoding="utf-8")
        rel = md.relative_to(DOCS).as_posix()
        stem = md.stem
        if re.match(r"\s*---.*?orphan:\s*true.*?---", text, re.S):
            continue
        # a page inside a `Name/` dir is a method page -> implied by its class
        if md.parent.name in type_stems and md.parent.with_suffix(".md").exists():
            continue
        if stem in NON_METHOD:
            continue
        has_toc = "{toctree}" in text
        title = title_of(text) or stem
        fam_path = md.parent.relative_to(API).as_posix()

        if has_toc and stem[:1].islower():
            m = TOCBLOCK.search(text)
            depth = 2
            if m:
                dm = re.search(r":maxdepth:\s*(\d+)", m.group(1))
                depth = int(dm.group(1)) if dm else 2
            families.append(Family(
                path=md.relative_to(API).with_suffix("").as_posix(),
                page=rel, title=stem, maxdepth=depth,
                children=[c for c in toctree_refs(text)],
            ))
            continue

        kind = "options" if stem.endswith("Options") else "function" if stem[:1].islower() else "class"
        methods: list[str] = []
        if kind == "class":
            mdir = md.with_suffix("")
            if mdir.is_dir():
                methods = sorted(p.stem for p in mdir.glob("*.md") if p.stem not in NON_METHOD)
            elif has_toc:
                methods = [Path(r).name for r in toctree_refs(text) if Path(r).name not in NON_METHOD]
        cpp = re.search(r"```cpp\n(.*?)```", text, re.S)
        overloads = len(re.findall(r";\s*$", cpp.group(1), re.M)) if cpp else 1
        symbols.append(Symbol(
            qualname=title if "::" in title else f"{fw}::{stem}",
            kind=kind, family=fam_path, page=rel,
            summary=lede(text)[:200],
            methods=methods,
            fields=[h.split()[-1].strip("`") for h in H3.findall(text)] if kind == "options" else [],
            overloads=max(1, overloads),
        ))

    m = Manifest(fw, fw, f"PlussAI {fw} framework.",
                 f"<pluss/ai/{fw}.h>", f"PlussAI/include/pluss/ai/{fw}.h",
                 families, symbols)
    if MANIFEST.exists() and not args.force:
        sys.exit(f"{MANIFEST.name} exists; pass --force to overwrite the draft")
    MANIFEST.write_text(dump_manifest(m), encoding="utf-8")
    n_methods = sum(len(s.methods) for s in symbols)
    print(f"wrote {MANIFEST.name}: {len(families)} families, {len(symbols)} symbols, "
          f"{n_methods} method pages (draft -- hand-tune it)")
    return 0


# --------------------------------------------------------------------------- #
# scaffold
# --------------------------------------------------------------------------- #
def rel_up(page: Path, target: str) -> str:
    depth = len(page.relative_to(DOCS).parts) - 1
    return "../" * depth + target


EXAMPLE_PREAMBLE = [
    "```cpp",
    "#include <pluss/pluss.h>",
    "#include <pluss/ai/lymph.h>",
    "",
    "#include <iostream>",
    "",
    "int main() {",
    "  namespace lymph = pluss::ai::lymph;",
    "  using pluss::ScalarType;",
    "  using pluss::table::DataFrame;",
    "  using pluss::table::Thread;",
    "",
    "  // TODO: construct realistic inputs, configure the object, run the call,",
    "  //       then print the result with std::cout.",
    "",
    "  return 0;",
    "}",
    "```",
    "",
    "Output",
    "",
    "```text",
    "TODO: paste the verified stdout of the exact program above",
    "```",
    "",
]


def _examples_block(n: int) -> list[str]:
    out = ["## Practical Examples", "",
           "<!-- one short paragraph: what these examples establish and demonstrate -->", ""]
    intents = ["baseline usage", "an important option or variation",
               "an end-to-end / workflow-level run", "an edge case or quirk",
               "an interpretation or failure-mode difference"]
    for i in range(1, n + 1):
        out += [f"### Example {i}", "",
                f"<!-- one paragraph: why this example exists ({intents[min(i - 1, len(intents) - 1)]}) -->", ""]
        out += EXAMPLE_PREAMBLE
    return out


def method_skeleton(sym: Symbol, method: str) -> str:
    L = [HAND, "<!-- pluss-signature-hash: 000000000000 -->", "",
         f"# `{sym.qualname}::{method}`", "",
         f"[Back to `{sym.qualname}`](../{sym.name}.md)", "",
         "## Signature", "", "```cpp",
         "// public callable declarations only -- all overloads, incl. the `_` in-place",
         "// form when this method has one; full parameter types and defaults.", "```", "",
         "## Parameters", "",
         "<!-- deflist: for each parameter, what it controls, its default, and the",
         "     behaviour or interpretation consequence of changing it. -->",
         "`<Type name>`", ": <!-- TODO -->", "",
         "## Returns", "",
         "<!-- what the value form returns; add a `Mutates:` line for the `_` form -->", "",
         "## Errors", "", "- <!-- preconditions and failure modes, or delete -->", ""]
    L += _examples_block(3)
    L += ["## See Also", "",
          "<!-- sibling method pages on this type, one line each -->", "",
          NAV_START, NAV_END, ""]
    return "\n".join(L)


def class_landing_skeleton(sym: Symbol) -> str:
    L = [f"# `{sym.qualname}`", "",
         sym.summary or "<!-- one or two sentences: what this object is -->", "",
         "## Signature", "", "```cpp",
         "// the public factories / constructors that produce this object",
         "// e.g. lymph::models::Model lymph::models::logistic_regression(...);", "```", "",
         "<!-- Objects with method pages: link the Introduction child for role "
         "prose, list methods below. Objects whose methods fold into this page: "
         "no child toctree; carry role / contract / notes as sections here. -->", "",
         TOC_START, TOC_END, ""]
    return "\n".join(L)


def options_skeleton(sym: Symbol) -> str:
    L = [f"# `{sym.qualname}`", "",
         sym.summary or "<!-- one sentence on what this options object configures -->", "",
         "<!-- one paragraph on the role of this bag: which call consumes it and when -->", "",
         "## Fields", ""]
    for f in sym.fields or ["<type> <field>"]:
        L += [f"### `{f}`", "",
              "<!-- type + name, what it controls, default value, what changes when",
              "     it is toggled, and when to prefer one setting over another -->", ""]
    L += ["## Guidance", "",
          "<!-- how the defaults fit together; which fields to touch only with a reason -->", "",
          "## See Also", "", "<!-- the consuming symbol(s) -->", ""]
    return "\n".join(L)


def function_skeleton(sym: Symbol) -> str:
    L = [f"# `{sym.qualname}`", "",
         sym.summary or "<!-- one or two sentences: what this function does -->", "",
         "## Signature", "", "```cpp",
         "// public callable declarations only -- every overload, full types + defaults", "```", "",
         "## Parameters", "",
         "`<Type name>`", ": <!-- what it controls, default, effect of changing it -->", "",
         "## Returns", "", "<!-- the returned object and how to read it -->", "",
         "## Errors", "", "- <!-- preconditions and failure modes, or delete -->", ""]
    L += _examples_block(max(3, sym.min_examples))
    L += ["## See Also", "", "<!-- sibling symbols, one line each -->", ""]
    return "\n".join(L)


def cmd_scaffold(args: argparse.Namespace) -> int:
    m = load_manifest()
    made = 0

    def write(path: Path, content: str) -> None:
        nonlocal made
        if path.exists() and not args.force:
            return
        path.parent.mkdir(parents=True, exist_ok=True)
        verb = "overwrote" if path.exists() else "created  "
        path.write_text(content, encoding="utf-8")
        print(f"{verb} {path.relative_to(DOCS)}")
        made += 1

    for s in m.symbols:
        if args.family and not s.family.startswith(args.family):
            continue
        page = DOCS / s.page
        if s.kind == "class":
            write(page, class_landing_skeleton(s))
            # An Introduction child page exists only for objects that also have
            # method pages (the object page is then a hub and the role prose
            # needs its own home). When methods are folded into the object page,
            # its role / contract / notes live as sections of that page.
            if s.methods:
                write(s.method_dir / "Introduction.md",
                      f"# `{s.qualname}` Introduction\n\n"
                      "<!-- the object's role: what it owns, its lifecycle and state, "
                      "how it is obtained, and how it relates to neighbouring types. "
                      "Fold notes, gotchas, and any single-accessor contract detail in "
                      "here as trailing `## ...` sections rather than separate pages -->\n")
            for mth in s.methods:
                write(s.method_page(mth), method_skeleton(s, mth))
        elif s.kind == "options":
            write(page, options_skeleton(s))
        else:
            write(page, function_skeleton(s))
    print(f"{made} page(s) written")
    return 0


# --------------------------------------------------------------------------- #
# wire
# --------------------------------------------------------------------------- #
def _replace_block(text: str, start: str, end: str, body: str) -> str:
    block = f"{start}\n{body}\n{end}"
    if start in text:
        return re.sub(re.escape(start) + r".*?" + re.escape(end), lambda _m: block, text, flags=re.S)
    if "```{toctree}" in text and start == TOC_START:
        return re.sub(r"```\{toctree\}.*?```", lambda _m: block, text, count=1, flags=re.S)
    return text.rstrip() + "\n\n" + block + "\n"


def _toctree(refs: list[str], maxdepth: int, hidden: bool) -> str:
    head = ["```{toctree}", f":maxdepth: {maxdepth}"]
    if hidden:
        head.append(":hidden:")
    return "\n".join(head + [""] + refs + ["```"])


def _nav(prev: tuple[str, str] | None, nxt: tuple[str, str] | None) -> str:
    parts = ['<div class="pluss-inline-prev-next">']
    if prev:
        parts += [f'  <a class="prev-link" href="{prev[0]}.html">',
                  '    <span class="label">Previous</span>',
                  f'    <span class="title">{prev[1]}</span>', "  </a>"]
    if nxt:
        parts += [f'  <a class="next-link" href="{nxt[0]}.html">',
                  '    <span class="label">Next</span>',
                  f'    <span class="title">{nxt[1]}</span>', "  </a>"]
    parts.append("</div>")
    return "\n".join(parts)


def cmd_wire(args: argparse.Namespace) -> int:
    m = load_manifest()
    changed = 0

    def apply(page: Path, new: str, label: str) -> None:
        nonlocal changed
        old = page.read_text(encoding="utf-8") if page.exists() else None
        if old == new:
            return
        changed += 1
        if args.check:
            print(f"  would rewire {label}")
        else:
            page.write_text(new, encoding="utf-8")
            print(f"  rewired {label}")

    # Every toctree is emitted :hidden:. The left-rail nav (apiref_nav.js) is
    # the single place children are listed; repeating them as a bullet list in
    # the page body is noise.
    for fam in m.families:
        if args.family and not fam.path.startswith(args.family):
            continue
        page = DOCS / fam.page
        if not page.exists():
            print(f"  ! missing family page {fam.page}")
            continue
        text = page.read_text(encoding="utf-8")
        body = _toctree(fam.children, fam.maxdepth, hidden=True)
        apply(page, _replace_block(text, TOC_START, TOC_END, body), fam.page)

    for s in m.symbols:
        if args.family and not s.family.startswith(args.family):
            continue
        if s.kind != "class":
            continue
        page = DOCS / s.page
        stem = s.page[:-3]
        # type-tree order: Introduction (only for objects with method pages),
        # then method pages A-Z. An object whose methods are folded into its own
        # page carries no Introduction child and no child toctree at all.
        seq: list[str] = []
        if s.methods and (DOCS / f"{stem}/Introduction.md").exists():
            seq.append("Introduction")
        seq += sorted(s.methods)
        # Owned options pages nest under the object. They live beside the object
        # page (api/.../scaling/StandardScalerOptions.md), so their toctree ref
        # is a bare sibling stem, not a `{name}/...` child path.
        owned_opts = sorted(o.name for o in m.symbols
                            if o.kind == "options" and o.owner == s.qualname)
        if page.exists():
            refs = [f"{s.name}/{child}" for child in seq] + owned_opts
            body = _toctree(refs, 1, hidden=True) if refs else ""
            apply(page, _replace_block(page.read_text(encoding="utf-8"),
                                       TOC_START, TOC_END, body), s.page)
        # prev/next nav across the whole page sequence
        for i, child in enumerate(seq):
            mp = s.method_dir / f"{child}.md"
            if not mp.exists():
                continue
            prev = (seq[i - 1], seq[i - 1]) if i else None
            nxt = (seq[i + 1], seq[i + 1]) if i + 1 < len(seq) else None
            new = _replace_block(mp.read_text(encoding="utf-8"), NAV_START, NAV_END, _nav(prev, nxt))
            apply(mp, new, f"{stem}/{child}.md")

    if not args.check:
        _write_nav(m)

    if args.check and changed:
        return 1
    print(f"{changed} page(s) {'need rewiring' if args.check else 'rewired'}")
    return 0


# --------------------------------------------------------------------------- #
# nav  --  bundle nav data + renderer + styles into _static/api_nav.js
#
# api_nav.js is already in conf.py's html_js_files (it always has been), so
# regenerating *that* file means the new sidebar ships without touching conf
# and shows up on a plain page reload -- no rebuild-order gotchas.
# --------------------------------------------------------------------------- #
NAV_JS = DOCS / "_static" / "api_nav.js"
NAV_RENDERER = DOCS / "apiref_assets" / "sidebar.js"
NAV_STYLES = DOCS / "apiref_assets" / "sidebar.css"


def _href(page_md_or_rel: str) -> str:
    """`api/lymph/models/Model.md` -> `api/lymph/models/Model.html`."""
    p = page_md_or_rel
    if p.endswith(".md"):
        p = p[:-3] + ".html"
    elif not p.endswith(".html"):
        p = p + ".html"
    return p


def build_nav(m: Manifest) -> dict:
    # toctree refs are relative to the family PAGE's directory, so resolve every
    # ref to a real `api/...md` path and match families and symbols off that.
    fam_by_page = {f.page: f for f in m.families}
    sym_by_page = {s.page: s for s in m.symbols}
    sym_by_tail: dict[str, Symbol] = {}
    for s in m.symbols:
        sym_by_tail.setdefault(Path(s.page).stem, s)

    def symbol_node(s: Symbol) -> dict:
        node = {"type": s.kind, "label": s.name, "page": _href(s.page)}
        if s.kind != "class":
            return node
        stem = s.page[:-3]                       # api/.../Model
        kids: list[dict] = []
        # standard type-tree rhythm: Introduction, then method pages A-Z. The
        # Introduction node exists only for objects with method pages; an object
        # whose methods are folded into its own page has no child nodes at all,
        # and its role / contract prose lives as sections of that page.
        if s.methods and (DOCS / f"{stem}/Introduction.md").exists():
            kids.append({"type": "page", "label": "Introduction",
                         "page": _href(f"{stem}/Introduction")})
        for mth in sorted(s.methods):
            kids.append({"type": "method", "label": mth,
                         "page": _href(f"{stem}/{mth}")})
        # Owned options pages sit under the object they configure. The
        # indentation carries the "these options belong to this object"
        # relationship the flat family listing could not show.
        for o in sorted((x for x in m.symbols
                         if x.kind == "options" and x.owner == s.qualname),
                        key=lambda x: x.name):
            kids.append({"type": "options", "label": o.name,
                         "page": _href(o.page)})
        if kids:
            node["children"] = kids
        return node

    def family_node(fam: Family, seen: set[str]) -> dict:
        seen.add(fam.path)
        base = Path(fam.page).parent  # e.g. api/lymph  or  api/lymph/models
        node = {"type": "family", "label": fam.title, "page": _href(fam.page), "children": []}
        for ref in fam.children:
            ref_md = (base / f"{ref}.md").as_posix()
            tail = ref.split("/")[-1]

            child_fam = fam_by_page.get(ref_md)
            if child_fam and child_fam.path != fam.path and child_fam.path not in seen:
                node["children"].append(family_node(child_fam, seen))
                continue

            sym = sym_by_page.get(ref_md) or sym_by_tail.get(tail)
            if sym:
                node["children"].append(symbol_node(sym))
                continue

            if (DOCS / ref_md).exists():   # real page with no manifest entry
                node["children"].append(
                    {"type": "page", "label": tail, "page": _href(ref_md)})
        # symbols that belong here but were not named in the toctree -- except
        # owned options pages, which are rendered nested under their object.
        listed = {c["label"] for c in node["children"]}
        for s in m.symbols:
            if s.family == fam.path and s.name not in listed and not s.owner:
                node["children"].append(symbol_node(s))
        return node

    seen: set[str] = set()
    roots = [family_node(f, seen) for f in m.families
             if f.path.count("/") == 1 and f.path not in seen]
    return {
        "title": f"{m.fw_title} API",
        "framework": m.framework,
        "root_page": "api_reference.html",
        "tree": roots,
    }


def _write_nav(m: Manifest) -> None:
    data = build_nav(m)
    css = NAV_STYLES.read_text(encoding="utf-8") if NAV_STYLES.exists() else ""
    renderer = NAV_RENDERER.read_text(encoding="utf-8") if NAV_RENDERER.exists() else ""
    css_js = json.dumps(css)  # safe string literal for the <style> injector
    bundle = (
        "/* GENERATED by `apiref.py wire` from apiref_manifest.toml + "
        "apiref_assets/. Do not edit. */\n"
        "window.APIREF_NAV = " + json.dumps(data, indent=2) + ";\n\n"
        "(function () {\n"
        "  try {\n"
        "    var s = document.createElement('style');\n"
        "    s.id = 'apiref-nav-styles';\n"
        "    s.textContent = " + css_js + ";\n"
        "    (document.head || document.documentElement).appendChild(s);\n"
        "  } catch (e) {}\n"
        "})();\n\n"
        + renderer + "\n"
    )
    NAV_JS.parent.mkdir(parents=True, exist_ok=True)
    NAV_JS.write_text(bundle, encoding="utf-8")
    n = _count_nodes(data["tree"])
    print(f"  wrote _static/{NAV_JS.name} ({n} nav entries, "
          f"{len(css)} B css, {len(renderer)} B renderer)")


def _count_nodes(nodes: list[dict]) -> int:
    return sum(1 + _count_nodes(x.get("children", [])) for x in nodes)


def cmd_nav(args: argparse.Namespace) -> int:
    _write_nav(load_manifest())
    return 0


# --------------------------------------------------------------------------- #
# sighash
# --------------------------------------------------------------------------- #
def cmd_sighash(args: argparse.Namespace) -> int:
    m = load_manifest()
    header_text = read_header_surface(m)
    if header_text is None:
        print(f"note: public header {m.fw_header} not found; "
              f"hashing each page's own ## Signature block instead")

    stale = 0
    for s in m.symbols:
        if s.kind != "class":
            continue
        for mth in s.methods:
            mp = s.method_page(mth)
            if not mp.exists():
                continue
            text = mp.read_text(encoding="utf-8")
            want = sig_hash(text)
            mm = SIGHASH_RE.search(text)
            have = mm.group(1) if mm else None
            if have == want:
                continue
            stale += 1
            if args.write:
                if mm:
                    text = SIGHASH_RE.sub(f"<!-- pluss-signature-hash: {want} -->", text, count=1)
                else:
                    text = HAND + f"\n<!-- pluss-signature-hash: {want} -->\n" + text
                mp.write_text(text, encoding="utf-8")
                print(f"  stamped {s.page[:-3]}/{mth}.md  {have} -> {want}")
            else:
                print(f"  STALE  {s.page[:-3]}/{mth}.md  marker={have} computed={want}")
    if stale and not args.write:
        print(f"{stale} stale signature hash(es); run `apiref.py sighash --write` after "
              f"reviewing each ## Signature")
        return 1
    print("signature hashes: clean" if not stale else f"{stale} stamped")
    return 0


# --------------------------------------------------------------------------- #
# lint
# --------------------------------------------------------------------------- #
def section_errors(kind: str, present: list[str]) -> list[str]:
    spec = SECTIONS.get(kind, [])
    names = [x.lstrip("*") for x in spec]
    required = [x.lstrip("*") for x in spec if x.startswith("*")]
    errs = [f"missing required section: ## {r}" for r in required if r not in present]
    seq = [p for p in present if p in names]
    if seq != sorted(seq, key=names.index):
        errs.append(f"sections out of order: {seq} (want {names})")
    return errs


def cmd_lint(args: argparse.Namespace) -> int:
    m = load_manifest()
    probs: list[str] = []
    known_pages = {s.page for s in m.symbols} | {f.page for f in m.families}
    for s in m.symbols:
        for mth in s.methods:
            known_pages.add(f"{s.page[:-3]}/{mth}.md")

    header_text = read_header_surface(m)

    def check_forbidden(rel: str, text: str) -> None:
        low = text.lower()
        for pat in FORBIDDEN_TERMS:
            mobj = re.search(pat, low)
            if mobj:
                probs.append(f"{rel}: forbidden term \"{mobj.group(0)}\" "
                             f"(no classroom framing, cross-library comparison, or internal names)")

    def check_signature_discipline(rel: str, text: str) -> None:
        mobj = re.search(r"##\s+Signature\s*\n+```cpp\n(.*?)```", text, re.S)
        if not mobj:
            return
        block = mobj.group(1)
        for marker in SIG_BODY_MARKERS:
            if marker in block:
                probs.append(f"{rel}: ## Signature contains `{marker.strip()}` "
                             f"-- show public callable declarations, not a pasted body")
                break

    def check_examples(rel: str, text: str) -> None:
        mobj = re.search(r"##\s+Practical Examples\b(.*?)(?:\n##\s|\Z)", text, re.S)
        if not mobj:
            probs.append(f"{rel}: no ## Practical Examples section")
            return
        body = mobj.group(1)
        blocks = re.split(r"\n###\s+Example\s+\d+\b", body)[1:]
        if len(blocks) < 3:
            probs.append(f"{rel}: {len(blocks)} numbered example(s) under "
                         f"## Practical Examples, need >= 3 (Example 1..N)")
        for i, blk in enumerate(blocks, 1):
            if "int main(" not in blk:
                probs.append(f"{rel}: Example {i} is not an end-to-end `int main()` program")
            if "std::cout" not in blk and ".writeln(" not in blk:
                probs.append(f"{rel}: Example {i} has no explicit print statement")
            if not re.search(r"```\s*\n+Output\b", blk) and "\nOutput\n" not in blk:
                probs.append(f"{rel}: Example {i} has no verified `Output` block beneath it")

    def check_page(rel: str, kind: str, sym: Symbol | None, method: str | None) -> None:
        page = DOCS / rel
        if not page.exists():
            probs.append(f"{rel}: in manifest but missing (run scaffold)")
            return
        text = page.read_text(encoding="utf-8")
        check_forbidden(rel, text)
        for e in section_errors(kind, h2s(text)):
            probs.append(f"{rel}: {e}")

        if kind in ("method", "function"):
            check_signature_discipline(rel, text)
            check_examples(rel, text)
            # A parameter counts as documented if it is named anywhere the page
            # explains inputs -- a ## Parameters deflist, or a prose section such
            # as ## Input Shapes for methods with several (features, label) forms.
            documented = "".join(re.findall(
                r"##\s+(?:Parameters|Input Shapes|Fields)\b(.*?)(?:\n##\s|\Z)", text, re.S))
            for p in sig_params(text):
                if not re.search(rf"[`&\s]{re.escape(p)}[`,\s]", documented):
                    probs.append(f"{rel}: signature parameter `{p}` not documented "
                                 f"(add it to ## Parameters or ## Input Shapes)")
        if kind == "method":
            mm = SIGHASH_RE.search(text)
            if not mm:
                probs.append(f"{rel}: missing <!-- pluss-signature-hash --> marker (line 2)")
            elif mm.group(1) != sig_hash(text):
                probs.append(f"{rel}: signature-hash marker is stale (run `apiref.py sighash --write`)")
        if kind == "class":
            check_signature_discipline(rel, text)
            if header_text is not None and sym and sym.name not in header_text:
                probs.append(f"{rel}: `{sym.name}` not found in public header {m.fw_header}")
        if kind == "options" and sym:
            check_signature_discipline(rel, text)
            h3 = {h.split()[-1].strip("`") for h in H3.findall(text)}
            for fld in sym.fields:
                if fld not in h3:
                    probs.append(f"{rel}: options field `{fld}` has no ### prose subsection")

    for s in m.symbols:
        if args.family and not s.family.startswith(args.family):
            continue
        check_page(s.page, s.kind, s, None)
        stem = s.page[:-3]
        for extra in ("Introduction", "Notes"):
            p = f"{stem}/{extra}.md"
            if (DOCS / p).exists():
                check_page(p, "page", s, None)
        for mth in s.methods:
            check_page(f"{stem}/{mth}.md", "method", s, mth)

    # stray pages under api/<fw>/ that the manifest does not know about
    known_pages |= {f"{s.page[:-3]}/{x}.md" for s in m.symbols for x in ("Introduction", "Notes")}
    # a family / subfamily landing may carry its own Introduction and Notes pages
    known_pages |= {f"{f.page[:-3]}/{x}.md" for f in m.families for x in ("Introduction", "Notes")}
    for md in sorted((API / m.framework).rglob("*.md")):
        rel = md.relative_to(DOCS).as_posix()
        text = md.read_text(encoding="utf-8")
        if "orphan: true" in text:
            continue
        check_forbidden(rel, text)
        if rel not in known_pages:
            probs.append(f"{rel}: page exists but has no manifest entry")

    # family children that do not resolve (refs are relative to the family PAGE)
    for fam in m.families:
        for child in fam.children:
            base = DOCS / Path(fam.page).parent / child
            if not base.with_suffix(".md").exists() and not base.is_dir():
                probs.append(f"{fam.page}: toctree child `{child}` has no page or directory")

    if probs:
        print(f"apiref lint: {len(probs)} problem(s)\n", file=sys.stderr)
        for p in probs:
            print(f"  - {p}", file=sys.stderr)
        return 1
    print("apiref lint: clean")
    return 0


# --------------------------------------------------------------------------- #
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("import", help="seed apiref_manifest.toml from the existing tree")
    p.add_argument("--family", required=True)
    p.add_argument("--force", action="store_true")
    p.set_defaults(fn=cmd_import)

    p = sub.add_parser("scaffold", help="create missing pages from the manifest")
    p.add_argument("--family", default="")
    p.add_argument("--force", action="store_true", help="overwrite existing pages too")
    p.set_defaults(fn=cmd_scaffold)

    p = sub.add_parser("wire", help="regenerate toctree + prev/next blocks + _static/apiref_nav.js")
    p.add_argument("--family", default="")
    p.add_argument("--check", action="store_true", help="report only; nonzero if stale")
    p.set_defaults(fn=cmd_wire)

    p = sub.add_parser("nav", help="(re)write _static/apiref_nav.js only")
    p.set_defaults(fn=cmd_nav)

    p = sub.add_parser("sighash", help="check or --write signature-hash markers")
    p.add_argument("--write", action="store_true")
    p.set_defaults(fn=cmd_sighash)

    p = sub.add_parser("lint", help="enforce the standards contracts")
    p.add_argument("--family", default="")
    p.set_defaults(fn=cmd_lint)

    args = ap.parse_args()
    return args.fn(args)


if __name__ == "__main__":
    raise SystemExit(main())
