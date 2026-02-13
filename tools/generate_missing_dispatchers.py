#!/usr/bin/env python3
import re
from pathlib import Path
ROOT = Path('/Users/yawasante/software/cxx/PlussMa')
DECL = ROOT / 'PlussQit/src/CodeM/core/SeriesMethods.inc'
IMPL = ROOT / 'PlussQit/src/CodeM/core/SeriesMethods.cpp.inc'

if not DECL.exists():
    print('Missing declarations file', DECL)
    raise SystemExit(1)
if not IMPL.exists():
    print('Missing impl file', IMPL)
    raise SystemExit(1)

decl_text = DECL.read_text(encoding='utf-8')
impl_text = IMPL.read_text(encoding='utf-8')

# helpers to extract names
def decl_names(text):
    return set(m.group(1) for m in re.finditer(r'\bSeries\b\s*(?:&\s*)?([A-Za-z0-9_]+)\s*\(', text))

# Build a list of all declaration signatures (including overloads)
decl_frags = [f.strip()+';' for f in re.split(r';\s*(?=[^\n])', decl_text) if f.strip()]
decl_sigs = []
for frag in decl_frags:
    m = re.search(r"\bSeries\b\s*(?:&\s*)?([A-Za-z0-9_]+)\s*\((.*?)\)\s*([^;]*);", frag, flags=re.S)
    if not m:
        continue
    name = m.group(1)
    params_raw = m.group(2).strip()
    qualifier = m.group(3).strip()
    params = '(' + params_raw + ')'
    decl_sigs.append((name, params, qualifier, params_raw))

# For each declaration signature, check whether an exact impl (name+params) exists; generate missing ones
out_impls = []
for name, params, qualifier, params_raw in decl_sigs:
    # build a simple regex to check for an existing impl with same param types/count; allow whitespace differences
    # We check for the name and the parameter list text (approx)
    impl_pattern = rf"Series(?:\s*&)?\s+Series::{re.escape(name)}\s*\({re.escape(params_raw)}\)\s*{re.escape(qualifier)}"
    if qualifier == '':
        # relax match for empty qualifier
        impl_pattern = rf"Series(?:\s*&)?\s+Series::{re.escape(name)}\s*\({re.escape(params_raw)}\)\s*\{{"
    if re.search(impl_pattern, impl_text, flags=re.S):
        continue
    # extract parameter names for forward call
    args = []
    if params_raw:
        parts = [p.strip() for p in re.split(r',\s*(?![^()]*\))', params_raw) if p.strip()]
        for p in parts:
            last = p.split()[-1]
            name_only = last.split('=')[0]
            name_only = name_only.rstrip('&*')
            name_only = name_only.strip()
            if name_only:
                args.append(name_only)
    args_str = ', '.join(args)
    call_args = ('*this' + (', ' + args_str if args_str else ''))
    qual_str = (' ' + qualifier) if qualifier else ''
    signature = f"Series Series::{name}{params}{qual_str} {{ return pluss::codem::ops_internal::{name}({call_args}); }}"
    out_impls.append(signature)
# backup original
bak = IMPL.with_suffix('.cpp.inc.bak')
if not bak.exists():
    bak.write_text(impl_text, encoding='utf-8')

# append
with IMPL.open('a', encoding='utf-8') as f:
    f.write('\n// Appended missing dispatchers\n')
    for s in out_impls:
        f.write(s + '\n')

print('Appended', len(out_impls), 'dispatchers to', IMPL)
