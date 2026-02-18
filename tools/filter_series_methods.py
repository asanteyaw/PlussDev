#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
INCL = ROOT / 'PlussQit/src/CodeM/Series/SeriesMethods.cpp.inc'
HEADER = ROOT / 'SeriesLowLevel.h'
OUT = INCL.with_name('SeriesMethods.filtered.cpp.inc')

hdr = HEADER.read_text(encoding='utf-8')
inline_names = set(re.findall(r"inline\s+Series\s+Series::([^(\s]+)\s*\(", hdr))

lines = INCL.read_text(encoding='utf-8').splitlines()
out = []
skipped = []
for l in lines:
    m = re.search(r"Series::([^(\s]+)\s*\(", l)
    if m:
        name = m.group(1)
        if name in inline_names:
            skipped.append(name)
            continue
    out.append(l)

OUT.write_text('\n'.join(out) + '\n', encoding='utf-8')
print('Wrote', OUT, 'skipped', len(skipped))
