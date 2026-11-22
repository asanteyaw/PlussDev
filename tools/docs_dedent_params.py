#!/usr/bin/env python3
"""Rewrite API-reference parameter entries so descriptions render as paragraphs.

Pattern fixed (a bold name line directly followed by an indented description,
which CommonMark folds into one run-on paragraph):

    **y_feature** (`std::string_view`)
        A single numeric feature.

Target layout:

    **y_feature** (`std::string_view`)

    A single numeric feature.

The transformation is idempotent. Fenced blocks (``` or ~~~) at the top level
are never touched; a fenced block inside an indented description is dedented
together with the description (and its content is otherwise byte-identical).

Usage: tools/docs_dedent_params.py [--check] [--exclude-file LIST] [paths...]
With no paths, scans the PlussPl/PlussMic/PlussQit/PlussMa/PlussAI/PlussTils
docs trees for *.md and *.md.in files.
"""
import os
import re
import sys

LIBS = "PlussPl PlussMic PlussQit PlussMa PlussAI PlussTils".split()
SKIP_DIRS = {"_build", "_verified", ".doctrees", "node_modules", ".git"}
NAME_RE = re.compile(r"\*\*[^*\n]+\*\*")
MARKER_RE = re.compile(r"^(#|\||[-*+] |>|`{3}|~{3}|:{3}|<|\d+[.)] |\*\*)")
HEADER_RE = re.compile(r"^\*\*[^*]+\*\*(, \*\*[^*]+\*\*)* \(.*\)\s*$")
FENCE_RE = re.compile(r"^ {0,3}(`{3,}|~{3,})")


def indent_width(line):
    w = 0
    for ch in line:
        if ch == " ":
            w += 1
        elif ch == "\t":
            w += 4 - (w % 4)
        else:
            break
    return w


def is_indented(line):
    return line.strip() != "" and indent_width(line) >= 4


def strip_cols(line, n):
    """Remove n columns of leading whitespace (tabs count to the next 4 stop)."""
    w = 0
    i = 0
    while i < len(line) and w < n:
        ch = line[i]
        if ch == " ":
            w += 1
        elif ch == "\t":
            nw = w + 4 - (w % 4)
            if nw > n:  # partial tab: replace by remaining spaces
                return " " * (nw - n) + line[i + 1:]
            w = nw
        else:
            break
        i += 1
    return line[i:]


def transform(text):
    lines = text.split("\n")
    out = []
    i = 0
    n = len(lines)
    fence = None
    count = 0
    while i < n:
        line = lines[i]
        m = FENCE_RE.match(line)
        if m:
            tok = m.group(1)
            if fence is None:
                fence = tok
            elif tok[0] == fence[0] and len(tok) >= len(fence) and line.strip() == tok:
                fence = None
            out.append(line)
            i += 1
            continue
        if fence is not None:
            out.append(line)
            i += 1
            continue
        h = i + 1  # first line after the (possibly wrapped) name line
        if NAME_RE.match(line):
            while (h < n and h - i < 5 and lines[h].strip()
                   and not is_indented(lines[h]) and not MARKER_RE.match(lines[h])):
                h += 1
        if NAME_RE.match(line) and h < n and is_indented(lines[h]):
            # collect the continuation block
            j = h
            block = []
            while j < n:
                l = lines[j]
                if l.strip() == "":
                    # blank: continue only if the next non-blank is indented
                    k = j
                    while k < n and lines[k].strip() == "":
                        k += 1
                    if k < n and is_indented(lines[k]):
                        block.extend(lines[j:k])
                        j = k
                        continue
                    break
                if not is_indented(l):
                    break
                block.append(l)
                j += 1
            common = min(indent_width(b) for b in block if b.strip())
            ded = [strip_cols(b, common) if b.strip() else "" for b in block]
            out.extend(lines[i:h])
            out.append("")
            out.extend(ded)
            # blank line before whatever follows
            if j < n and lines[j].strip() != "":
                out.append("")
            count += 1
            i = j
            continue
        if (HEADER_RE.match(line) and i + 1 < n and lines[i + 1].strip()
                and not is_indented(lines[i + 1]) and not MARKER_RE.match(lines[i + 1])):
            # description already unindented but glued to the name line: separate it
            out.append(line)
            out.append("")
            count += 1
            i += 1
            continue
        out.append(line)
        i += 1
    return "\n".join(out), count


def iter_files(paths):
    for p in paths:
        if os.path.isfile(p):
            yield p
            continue
        for r, d, fs in os.walk(p):
            d[:] = sorted(x for x in d if x not in SKIP_DIRS)
            for f in sorted(fs):
                if f.endswith((".md", ".md.in")):
                    yield os.path.join(r, f)


def main(argv):
    check = "--check" in argv
    excl = set()
    args = []
    it = iter(argv)
    for a in it:
        if a == "--check":
            continue
        if a == "--exclude-file":
            with open(next(it)) as fh:
                excl |= {x.strip() for x in fh if x.strip()}
            continue
        args.append(a)
    paths = args or [lib + "/docs" for lib in LIBS]
    total = files = 0
    for f in iter_files(paths):
        if f in excl:
            continue
        with open(f, encoding="utf8", newline="") as fh:
            src = fh.read()
        if "\r" in src:
            continue
        new, c = transform(src)
        if c:
            total += c
            files += 1
            print(f"{f}: {c}")
            if not check and new != src:
                with open(f, "w", encoding="utf8", newline="") as fh:
                    fh.write(new)
    print(f"{files} files, {total} occurrences")
    return 1 if (check and total) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
