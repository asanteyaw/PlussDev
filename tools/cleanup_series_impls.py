#!/usr/bin/env python3
"""
Rebuild SeriesMethods.cpp.inc so it contains exactly one implementation per declaration
(including overloads), in the same order as declarations. Removes extraneous/duplicate
impls that do not correspond to a declaration.
"""
from pathlib import Path
import re
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

# Extract declaration signatures (in order)
# Match Series [maybe &] name ( params ) [qualifiers] ;
decl_pattern = re.compile(r"\bSeries\b\s*(?:&\s*)?([A-Za-z0-9_]+)\s*\((.*?)\)\s*([^;]*);", re.S)
decl_sigs = []
for m in decl_pattern.finditer(decl_text):
    name = m.group(1)
    params_raw = m.group(2).strip()
    qualifier = m.group(3).strip()
    # normalize params spacing
    params_norm = ' '.join(re.split(r'\s+', params_raw))
    decl_sigs.append((name, params_norm, qualifier, params_raw))

# Parse existing impl functions (capture full text for preservation)
impl_pattern = re.compile(r"(Series(?:\s*&)?\s+Series::([A-Za-z0-9_]+)\s*\((.*?)\)\s*([^\{]*?)\{.*?\})", re.S)
impl_matches = []
for m in impl_pattern.finditer(impl_text):
    full = m.group(1)
    name = m.group(2)
    params_raw = m.group(3).strip()
    qualifier = m.group(4).strip()
    params_norm = ' '.join(re.split(r'\s+', params_raw))
    impl_matches.append({'name': name, 'params_raw': params_raw, 'params_norm': params_norm, 'qualifier': qualifier, 'full': full})

# Create index of impls by (name, params_norm, qualifier)
from collections import defaultdict, deque
impl_index = defaultdict(deque)
for im in impl_matches:
    key = (im['name'], im['params_norm'], im['qualifier'])
    impl_index[key].append(im['full'])

# Helper to generate a forwarder implementation for a declaration signature

def make_forwarder(name, params_raw, qualifier):
    params = '(' + params_raw + ')' if params_raw else '()'
    # extract parameter names
    args = []
    if params_raw:
        parts = [p.strip() for p in re.split(r',\s*(?![^()]*\))', params_raw) if p.strip()]
        for p in parts:
            last = p.split()[-1]
            name_only = last.split('=')[0]
            name_only = name_only.rstrip('&*').strip()
            if name_only:
                args.append(name_only)
    args_str = ', '.join(args)
    call_args = ('*this' + (', ' + args_str if args_str else ''))
    qual_str = (' ' + qualifier) if qualifier else ''
    return f"Series Series::{name}{params}{qual_str} {{ return pluss::codem::ops_internal::{name}({call_args}); }}"

# Build new impl list by iterating declarations in order
new_impls = []
used_impls = set()
for name, params_norm, qualifier, params_raw in decl_sigs:
    key = (name, params_norm, qualifier)
    if key in impl_index and impl_index[key]:
        # consume one existing impl (preserve its full text)
        full = impl_index[key].popleft()
        new_impls.append(full)
        used_impls.add((name, params_norm, qualifier, full))
    else:
        # try to find any impl with same name and qualifier ignoring params_norm (best-effort)
        alt_key_found = None
        for k in list(impl_index.keys()):
            if k[0] == name and k[2] == qualifier and impl_index[k]:
                alt_key_found = k
                break
        if alt_key_found:
            full = impl_index[alt_key_found].popleft()
            new_impls.append(full)
            used_impls.add((name, params_norm, qualifier, full))
        else:
            # generate forwarder
            fwd = make_forwarder(name, params_raw, qualifier)
            new_impls.append(fwd)

# Report any leftover impls that were not matched (these will be removed)
leftover = sum(len(q) for q in impl_index.values())
print('Found', len(decl_sigs), 'declaration occurrences')
print('Existing impl occurrences parsed:', len(impl_matches))
print('Leftover impl occurrences (to be removed):', leftover)

# Build output file: preserve leading header up to first impl occurrence, then write new impls
# Find position of first impl in original file to keep header
first_impl_match = impl_pattern.search(impl_text)
if first_impl_match:
    header = impl_text[:first_impl_match.start()]
else:
    header = '// Generated Series impls\n'

new_text = header.rstrip() + '\n\n// Cleaned/generated Series method implementations\n\n'
for s in new_impls:
    new_text += s.rstrip() + '\n'

# backup
bak = IMPL.with_suffix('.cpp.inc.bak')
if not bak.exists():
    bak.write_text(impl_text, encoding='utf-8')

IMPL.write_text(new_text, encoding='utf-8')
print('Wrote cleaned implementations to', IMPL)

# Final counts
impl_count_after = len(list(re.finditer(r"Series(?:\s*&)?\s+Series::[A-Za-z0-9_]+\s*\(.*?\)\s*\{", new_text, flags=re.S)))
print('implementation_occurrences_after:', impl_count_after)
if impl_count_after != len(decl_sigs):
    print('Warning: impl count after cleanup does not match declaration occurrences')
else:
    print('Success: impl count matches declaration occurrences')
