#!/usr/bin/env python3
import re
from pathlib import Path
ROOT = Path('/Users/yawasante/software/cxx/PlussMa')
DECL = ROOT / 'PlussQit/src/CodeM/core/SeriesMethods.inc'
IMPL = ROOT / 'PlussQit/src/CodeM/core/SeriesMethods.cpp.inc'

def extract_decl_names(text):
    names = set()
    # match lines like: Series name(...) ; or Series& name(...);
    for m in re.finditer(r'\bSeries\b\s*(?:&\s*)?([A-Za-z0-9_]+)\s*\(', text):
        names.add(m.group(1))
    return names


def extract_impl_names(text):
    names = set()
    # match Series ... Series::name( or Series& Series::name(
    for m in re.finditer(r'Series(?:\s*&)?\s+Series::([A-Za-z0-9_]+)\s*\(', text):
        names.add(m.group(1))
    return names

if not DECL.exists():
    print('Declaration file missing:', DECL)
    raise SystemExit(1)
if not IMPL.exists():
    print('Implementation file missing:', IMPL)
    raise SystemExit(1)

d = DECL.read_text(encoding='utf-8')
i = IMPL.read_text(encoding='utf-8')

decl_names = extract_decl_names(d)
impl_names = extract_impl_names(i)

missing = sorted(n for n in decl_names if n not in impl_names)
extra = sorted(n for n in impl_names if n not in decl_names)

print('Declarations:', len(decl_names))
print('Implementations:', len(impl_names))
print('Missing dispatchers for declarations:', len(missing))
for n in missing:
    print(n)

print('\nImpls that have no declaration:', len(extra))
for n in extra[:200]:
    print(n)

# return non-zero if missing
if missing:
    raise SystemExit(2)
else:
    print('\nAll declarations have dispatchers.')
