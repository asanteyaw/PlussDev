# Patch 63: Sort/Argsort/Topk + where() fix + more Torch-method routing

## Fix
- Corrected a missing closing brace in `ops_internal::where` that could break compilation / control flow.

## New CPU ops
- `Series::sort(dim, descending, stable)` -> returns `{values, indices}`
- `Series::argsort(dim, descending, stable)` -> returns indices
- `Series::topk(k, dim, largest, sorted)` -> returns `{values, indices}`

Implementation strategy:
- Permute the requested dim to last, make contiguous
- Sort (or nth_element for topk) per-row
- Undo permutation back to original dim order
- Indices are `int64`, matching Torch.

## Torch-method surface routing
`torch_compat::invoke_any` now routes:
- `argsort(dim)` and some multi-arg forms
- For generated `sort/topk`, the invoke layer returns **values** by default (since the generated wrapper returns `Series`),
  while the proper API is available via the manual `Series::sort/topk` methods returning pairs.

This keeps correctness while still making the generated surface “usable”.
