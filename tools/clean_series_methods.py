#!/usr/bin/env python3
import sys
from pathlib import Path

INPUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('PlussQit/src/CodeM/core/SeriesMethods.inc')
OUTPUT = Path(sys.argv[2]) if len(sys.argv) > 2 else INPUT.with_name('SeriesMethods.cleaned.inc')

# keywords indicating device/accelerator-specific or non-CPU methods to drop
DROP_KEYWORDS = [
    'cuda', 'mps', 'mkldnn', 'to_mkldnn', 'roc', 'hip', 'metal', 'to_mps', 'to_cuda',
    'to_sparse_csr', 'to_sparse_csc', 'to_sparse_bsr', 'to_sparse_bsc'
]

# additional heuristics: skip device-specific factory or quantization accessors
DROP_SUBSTRINGS = ['q_per_channel', 'qscheme', 'quantize', 'dequantize']

seen = set()
kept = []
skipped = []

with INPUT.open('r', encoding='utf-8') as f:
    for raw in f:
        line = raw.rstrip('\n')
        stripped = line.strip()
        if not stripped:
            continue
        low = stripped.lower()
        # drop lines that contain any drop keyword
        if any(k in low for k in DROP_KEYWORDS) or any(k in low for k in DROP_SUBSTRINGS):
            skipped.append(line)
            continue
        # normalize whitespace for signature-based dedupe
        norm = ' '.join(stripped.split())
        if norm in seen:
            skipped.append(line)
            continue
        seen.add(norm)
        kept.append(line)

with OUTPUT.open('w', encoding='utf-8') as out:
    out.write('// Cleaned: removed device-specific methods and duplicates\n')
    out.write(f'// Source: {INPUT}\n')
    out.write(f'// Kept: {len(kept)}, Skipped: {len(skipped)}\n\n')
    for l in kept:
        out.write(l + '\n')

print('Wrote', OUTPUT)
print('kept:', len(kept), 'skipped:', len(skipped))
