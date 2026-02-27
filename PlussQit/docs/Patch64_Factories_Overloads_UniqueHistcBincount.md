# Patch 64: Factory overload expansion + unique/bincount/histc + masked_scatter + invoke rebuild

## Factories: `pluss::value` now supports:
- scalar integral (`pluss::make_series(2)`) -> Int64 0D series by default
- scalar bool (`pluss::make_series(true)`) -> Bool 0D series by default
- scalar double (`pluss::make_series(2.0)`) -> Float32 0D series by default
- nested `std::initializer_list` up to 7D, e.g.
  - `value({1,2,3})` (1D)
  - `value({{1,2},{3,4}})` (2D)
  - deeper nesting up to 7D, with ragged detection (throws)

Internals:
- shape inference + flatten in `ValueInitList.h`
- data routed through `Series::make_from_doubles` for dtype-correct materialization.

## New ops
- `Series::unique(sorted=true)` (CPU ref, 1D)
- `Series::bincount(minlength=0)` (CPU ref)
- `Series::histc(bins,min,max)` (CPU ref)
- `Series::masked_scatter(mask, src)` (+ inplace) (CPU ref, safe shape policy)

## Torch method backstop
`torch_compat::Invoke.cpp` was rebuilt to restore a correct split between `invoke_any` and `invoke_any_`
and to route the new ops (`unique`, `bincount`, `histc`, `masked_scatter`) plus prior mappings.

This fixes an accidental drift where the file had lost `invoke_any_` separation.
