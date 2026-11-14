# Series API Reference

`Series` is the tensor-style core object in PlussQit: numeric, boolean, shaped,
strided, and close enough to Torch semantics that most array instincts carry over.
This reference is being rewritten method-by-method into handwritten pages with
real prose, verified outputs, and explicit edge-case coverage.

The first handwritten batch below focuses on indexing, layout, and selection,
because that is where the old reference was both thinnest and easiest to
misread.

## Handwritten Coverage

### Shape Introspection

- [`size`](_local/PlussQit/sphinx/api/series/size.md): ask for one axis extent, including negative-dim
  indexing.
- [`sizes`](_local/PlussQit/sphinx/api/series/sizes.md): borrow the full current shape as a lightweight
  view.

### Layout and Views

- [`view`](_local/PlussQit/sphinx/api/series/view.md): reinterpret a contiguous layout without copying.
- [`reshape`](_local/PlussQit/sphinx/api/series/reshape.md): `view()` when possible, contiguous fallback
  when not.

### Scalar Position and Rectangular Selection

- [`pos`](_local/PlussQit/sphinx/api/series/pos.md): rank-checked scalar selection for 1D, 2D, and 3D
  series.
- [`slice`](_local/PlussQit/sphinx/api/series/slice.md): cheap positive-step slicing views.
- [`block`](_local/PlussQit/sphinx/api/series/block.md): 2D rectangular subview extraction.

### Dynamic Indexing

- [`extract`](_local/PlussQit/sphinx/api/series/extract.md): the full dynamic selector surface
  (`Slice`, scalar indices, `Ellipsis`, `NewAxis`, bool/int advanced indices).
- [`index`](_local/PlussQit/sphinx/api/series/index.md): the Torch-style indexing entrypoint, especially
  relevant when one or more advanced index tensors are involved.
- [`extract_put`](_local/PlussQit/sphinx/api/series/extract_put.md): non-mutating selector-based writes.
- [`extract_put_`](_local/PlussQit/sphinx/api/series/extract_put_.md): in-place selector-based writes.
- [`index_put_`](_local/PlussQit/sphinx/api/series/index_put_.md): in-place advanced-index and
  convenience-index writes.

## Broader Usage Guides

- [`series.md`](series.md): usage-first tour of common `Series` workflows.
- [`series_query_user_guide.md`](series_query_user_guide.md): deeper walkthroughs
  for `traverse`, `stat`, and `linearalg`.
- [`series_all_methods_usage.md`](series_all_methods_usage.md): the broad legacy
  cookbook while the handwritten rewrite expands.

```{toctree}
:maxdepth: 1
:hidden:

_local/PlussQit/sphinx/api/series/pos
_local/PlussQit/sphinx/api/series/size
_local/PlussQit/sphinx/api/series/sizes
_local/PlussQit/sphinx/api/series/view
_local/PlussQit/sphinx/api/series/reshape
_local/PlussQit/sphinx/api/series/slice
_local/PlussQit/sphinx/api/series/block
_local/PlussQit/sphinx/api/series/extract
_local/PlussQit/sphinx/api/series/extract_put
_local/PlussQit/sphinx/api/series/extract_put_
_local/PlussQit/sphinx/api/series/index
_local/PlussQit/sphinx/api/series/index_put_
```
