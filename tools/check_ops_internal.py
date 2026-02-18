#!/usr/bin/env python3
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'PlussQit/src/CodeM/Series/SeriesMethods.cpp.inc'
CAND = ROOT / 'PlussQit/src/CodeM/core/OpsInternal.cpp'
# also check generated ops files
GENERATED = [
    ROOT / 'PlussQit/build/generated/CodeM/ops/ops_generated.cpp',
    ROOT / 'PlussQit/build67/generated/CodeM/ops/ops_generated.cpp',
    ROOT / 'build/PlussQit/generated/CodeM/ops/ops_generated.cpp',
]
all_generated = [p for p in GENERATED if p.exists()]

text = GEN.read_text(encoding='utf-8')
names = sorted(set(re.findall(r'ops_internal::([A-Za-z0-9_]+)\s*\(', text)))

candidates = []
for p in [CAND] + all_generated:
    if p.exists():
        candidates.append((p, p.read_text(encoding='utf-8')))

missing = []
not_found = set()
for name in names:
    found = False
    # search for function name in candidate files
    pat = re.compile(r"\b" + re.escape(name) + r"\s*\(")
    for (p, t) in candidates:
        if pat.search(t):
            found = True
            break
    if not found:
        # also search whole repo for definitions
        for p in ROOT.rglob('*.cpp'):
            try:
                t = p.read_text(encoding='utf-8')
            except Exception:
                continue
            if pat.search(t):
                found = True
                break
    if not found:
        missing.append(name)

print('Checked', len(names), 'ops; missing:', len(missing))
if missing:
    for m in missing:
        print(m)
else:
    print('All ops found')
