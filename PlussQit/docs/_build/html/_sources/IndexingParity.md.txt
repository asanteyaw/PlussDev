# Indexing parity (Torch-inspired, CPU-only)

This document tracks our advanced indexing behavior relative to Torch.

## Implemented (Patch 50)
- Non-leading advanced indices via "permute advanced dims to front" rule.
- Mixed bool masks + integer indices:
  - 1D bool mask -> int64 positions (nonzero)
  - ND bool mask over a contiguous block -> nonzero -> multi int64 indices (replacing the block)
- `index_put` supports the same mixed/non-leading normalization by permuting to front, applying update, then permuting back.

## Still not complete parity
- Ellipsis / None / Python-style slice objects (C++ API would need dedicated index objects)
- Some corner-case broadcasting rules (especially multiple advanced index tensors with irregular shapes)
- Strided in-place updates beyond contiguous safety policy
