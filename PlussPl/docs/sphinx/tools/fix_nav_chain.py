#!/usr/bin/env python3
"""Rewrites the inline previous/next block of every page of Studio, Pane and Trace so that the chain is A-Z (the API documentation
standard). Run after adding or removing a page."""
import pathlib, re, sys
SPHINX = pathlib.Path(__file__).resolve().parent.parent
BLOCK = re.compile(r'<div class="pluss-inline-prev-next">.*?</div>\s*$', re.S)
for family in sys.argv[1:] or ["pane", "studio", "trace"]:
    d = SPHINX / "api" / family
    names = sorted(p.stem for p in d.glob("*.md"))
    changed = 0
    for i, n in enumerate(names):
        path = d / f"{n}.md"
        text = path.read_text()
        parts = ['<div class="pluss-inline-prev-next">']
        if i > 0:
            parts += [f'  <a class="prev-link" href="{names[i-1]}.html">', '    <span class="label">Previous</span>', f'    <span class="title">{names[i-1]}</span>', '  </a>']
        if i + 1 < len(names):
            parts += [f'  <a class="next-link" href="{names[i+1]}.html">', '    <span class="label">Next</span>', f'    <span class="title">{names[i+1]}</span>', '  </a>']
        parts.append('</div>')
        new = "\n".join(parts) + "\n"
        if BLOCK.search(text):
            updated = BLOCK.sub(new, text)
        else:
            updated = text.rstrip("\n") + "\n\n" + new
        if updated != text:
            path.write_text(updated)
            changed += 1
    print(family, len(names), "pages,", changed, "changed")
