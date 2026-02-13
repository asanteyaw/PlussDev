#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path('/Users/yawasante/software/cxx/PlussMa')
GEN = ROOT / 'PlussQit/src/CodeM/Series/SeriesMethods.cpp.inc'
OUT = ROOT / 'PlussQit/src/CodeM/Series/SeriesMethods.filtered.cpp.inc'

def collect_existing_names():
    names = set()
    for p in ROOT.rglob('*'):
        if p.is_file():
            try:
                text = p.read_text(encoding='utf-8')
            except Exception:
                continue
            # find definitions like 'Series Series::name(' or 'Series& Series::name('
            for m in re.finditer(r"Series\s*&?\s+Series::([A-Za-z0-9_]+)\s*\(", text):
                names.add(m.group(1))
    return names

existing = collect_existing_names()
gen_text = GEN.read_text(encoding='utf-8')
out_lines = []
skipped = []
for line in gen_text.splitlines():
    m = re.search(r"Series::([A-Za-z0-9_]+)\s*\(", line)
    if m:
        name = m.group(1)
        if name in existing and 'SeriesMethods.filtered.cpp.inc' not in str(GEN):
            skipped.append(name)
            continue
    out_lines.append(line)

OUT.write_text('\n'.join(out_lines) + '\n', encoding='utf-8')
print('Wrote', OUT, 'skipped', len(skipped))
