#!/usr/bin/env python3
import sys
import os
import re
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def iter_source_files(root):
    for p in root.rglob('*'):
        if p.is_file() and p.suffix in {'.h', '.hpp', '.hh', '.c', '.cpp', '.cc', '.cxx'}:
            yield p

inc_re = re.compile(r'#\s*include\s*[<"]([^>"]+)[>"]')

def main():
    if len(sys.argv) < 2:
        print('Usage: prepare_missing_pytorch_files.py /path/to/pytorch')
        sys.exit(2)
    pytorch_root = Path(sys.argv[1]).expanduser()
    if not pytorch_root.exists():
        print('Pytorch path not found:', pytorch_root)
        sys.exit(2)

    seen = {}
    for src in iter_source_files(ROOT):
        try:
            text = src.read_text(errors='ignore')
        except Exception:
            continue
        for m in inc_re.finditer(text):
            inc = m.group(1)
            # ignore common system includes
            if inc.startswith(('sys/', 'unistd.h', 'stdio.h', 'stdlib.h', 'string.h', 'stdint.h')):
                continue
            seen.setdefault(inc, set()).add(str(src.relative_to(ROOT)))

    # renaming map: repo name -> pytorch original
    rename_map = {
        'MSharp': 'c10',
        'PlussQit': 'ATen',
        'pluss': 'torch',
        'Series': 'Tensor',
        'SeriesBody.h': 'TensorBody.h',
    }

    def map_candidate_paths(inc_path):
        # return a list of possible original names to try in pytorch
        parts = inc_path.split('/')
        candidates = [inc_path]
        # try substituting known top-level parts
        for i, part in enumerate(parts):
            if part in rename_map:
                new = parts.copy()
                new[i] = rename_map[part]
                candidates.append('/'.join(new))
        # also try basename mapping
        base = os.path.basename(inc_path)
        if base in rename_map:
            candidates.append(os.path.join(os.path.dirname(inc_path), rename_map[base]))
        return candidates

    missing = {}
    candidates = {}
    for inc, locations in seen.items():
        # check if file exists in repo relative path
        repo_path = ROOT / inc
        if repo_path.exists():
            continue
        # else check if any file with same name exists anywhere in repo
        basename = os.path.basename(inc)
        found_elsewhere = list(ROOT.rglob(basename))
        if found_elsewhere:
            missing[inc] = {
                'status': 'found_elsewhere',
                'locations': [str(p.relative_to(ROOT)) for p in found_elsewhere],
                'referenced_from': list(locations)[:5]
            }
            continue
        # search in pytorch tree (prefer the aten/ subtree) for same filename or mapped equivalents
        search_roots = []
        aten_root = pytorch_root / 'aten'
        if aten_root.exists():
            search_roots.append(aten_root)
        else:
            search_roots.append(pytorch_root)

        pyt_found = []
        for root in search_roots:
            pyt_found.extend(list(root.rglob(basename)))
        if not pyt_found:
            # try mapping variants under the same search roots
            for mapped in map_candidate_paths(inc):
                mb = os.path.basename(mapped)
                for root in search_roots:
                    found = list(root.rglob(mb))
                    if found:
                        pyt_found.extend(found)
        if pyt_found:
            # dedupe and stringify
            candidates[inc] = sorted({str(p) for p in pyt_found})
        else:
            missing[inc] = {
                'status': 'not_found_anywhere',
                'referenced_from': list(locations)[:5]
            }

    out = {'candidates': candidates, 'missing_other': missing}
    dest = ROOT / 'scripts' / 'missing_candidates.json'
    dest.write_text(json.dumps(out, indent=2))
    print('Wrote', dest)
    print('Candidates to copy from pytorch:', len(candidates))
    print('Missing (not found anywhere or only found_elsewhere):', len(missing))

if __name__ == '__main__':
    main()
