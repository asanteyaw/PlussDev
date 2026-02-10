#!/usr/bin/env python3
"""Scan repository for #include directives and report missing include targets.

Prints file:line -> include and whether the target exists. Highlights likely PyTorch includes (ATen/torch/c10/MSharp).
"""
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXTS = {'.cpp', '.cc', '.c', '.cxx', '.h', '.hpp', '.hh', '.inl', '.ipp'}
include_re = re.compile(r'^[ \t]*#include[ \t]+[<"]([^>"]+)[>"]')

def candidate_paths(inc, src_file):
    # If quoted, prefer relative to src_file dir
    paths = []
    p = Path(inc)
    if inc.startswith('"') or inc.startswith("'"):
        pass
    # Common places to check
    paths.append(ROOT / inc)
    paths.append(ROOT / 'include' / inc)
    paths.append(ROOT / 'include' / inc.replace('ATen/', 'PlussQit/'))
    paths.append(ROOT / inc.replace('ATen/', 'PlussQit/'))
    # also check relative to source file
    paths.append(Path(src_file).parent / inc)
    return paths

def is_probably_system(inc):
    # Heuristic: no slash and lowercase
    if '/' not in inc and inc.lower() == inc:
        return True
    return False

missing = []
total = 0
for dirpath, dirnames, filenames in os.walk(ROOT):
    # skip build and .git
    if any(part in ('build', '.git') for part in Path(dirpath).parts):
        continue
    for fname in filenames:
        if Path(fname).suffix not in EXTS:
            continue
        fpath = Path(dirpath) / fname
        try:
            text = fpath.read_text(encoding='utf-8')
        except Exception:
            continue
        for i, line in enumerate(text.splitlines(), start=1):
            m = include_re.match(line)
            if not m:
                continue
            inc = m.group(1)
            total += 1
            # skip obvious system headers
            if is_probably_system(inc):
                continue
            found = False
            for cand in candidate_paths(inc, fpath):
                if cand.exists():
                    found = True
                    break
            if not found:
                kind = 'pytorch-ish' if any(k in inc for k in ('ATen/', 'torch/', 'c10/', 'ATen\'', 'ATen')) else 'missing'
                missing.append((str(fpath.relative_to(ROOT)), i, inc, kind))

if not missing:
    print('No missing includes found (non-system). Scanned includes:', total)
else:
    print('Missing includes (non-system):')
    for f, line, inc, kind in missing:
        print(f"{f}:{line} -> {inc}  [{kind}]")
    print('\nCount:', len(missing), 'scanned includes:', total)
