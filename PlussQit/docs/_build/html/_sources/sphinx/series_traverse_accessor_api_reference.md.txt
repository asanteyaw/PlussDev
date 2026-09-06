<!-- pluss-hand-authored -->

# SeriesTraverseAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

This is the landing page for the value-driven search workflow entered through
[`Series::traverse`](api/series/traverse.md). The accessor turns a condition on
a `Series`' values into the **coordinates** of every matching position, wrapped
in a queryable [`TraverseMatch`](api/series-traverse/match.md).

## Why This Family Is Its Own Surface

The rest of the `Series` API answers "reshape / reduce / transform every
element." `traverse` answers a different question: *which* elements satisfy a
predicate, and *where* are they. Ordinary comparisons give you a boolean
`Series` of the same shape and stop there; `traverse` carries the search
through to positions, matched values, flat offsets, and a gather helper — one
call, one result object.

This is one of the surfaces where `Series` intentionally diverges from a
plain tensor library, so it earns a dedicated reference.

## The Calling Model

Every finder is available two ways:

```cpp
// (1) directly on the accessor -- searches the whole Series
auto a = grid.traverse.find_ge(6.0);

// (2) through a proxy that first narrows to a view -- searches only that view
auto b = grid.traverse({std::int64_t{1}}).find_ge(5.0);   // row 1 only
```

The selector list passed to `traverse({ ... })` is the same view-only language
as [`traverse_slice()`](api/series/traverse_slice.md): scalar indices,
positive-step slices, `SliceAll()`, `NewAxis()`, `Ellipsis()`. When you use it,
the resulting match's coordinates and `rank()` are relative to the **sub-view**.

Both call styles return a [`TraverseMatch`](api/series-traverse/match.md).

## Method Pages

### Explicit mask

- [`find_all`](api/series-traverse/find_all.md): match every position where a
  boolean mask is true (`find_if` is an identical spelling)
- [`find_if`](api/series-traverse/find_if.md): alias of `find_all`

### Value predicates

- [`find_eq`](api/series-traverse/find_eq.md): match positions equal to a
  scalar or peer `Series`
- [`find_neq`](api/series-traverse/find_neq.md): the negation of `find_eq`
- [`find_ge`](api/series-traverse/find_ge.md): match positions at or above a
  threshold
- [`find_le`](api/series-traverse/find_le.md): match positions at or below a
  threshold
- [`find_btw`](api/series-traverse/find_btw.md): match positions inside a
  `[lo, hi]` band, inclusive or exclusive

### Mask algebra

- [`find_and`](api/series-traverse/find_and.md): match where two masks are both
  true
- [`find_or`](api/series-traverse/find_or.md): match where either mask is true
- [`find_not`](api/series-traverse/find_not.md): match where a mask is false

### Positional

- [`find_first`](api/series-traverse/find_first.md): keep only the first match
  globally, or the first match in each lane along an axis, or the first match in
  each leading batch

### Result object

- [`TraverseMatch`](api/series-traverse/match.md): `count`, `coords`, `values`,
  `linear_indices`, `split_indices`, `select_from`, and the shape/status
  queries

## Choosing A Finder

- Use `find_eq` / `find_ge` / `find_le` / `find_btw` when the condition is a
  direct comparison — they build the mask for you.
- Use `find_all` when you have already computed a mask, possibly with logic the
  comparison finders do not cover.
- Use `find_and` / `find_or` / `find_not` to combine two existing masks without
  a temporary `logical_*` call.
- Use `find_first` when you only want one representative match per row, per
  column, or per batch — for example the first crossing of a threshold in each
  sequence.
- Reach into [`TraverseMatch`](api/series-traverse/match.md) for what to do with
  the hits: count them, read their values (differentiable), turn them into flat
  offsets, or gather the same positions from a parallel `Series`.

```{toctree}
:hidden:
:maxdepth: 1

api/series-traverse/find_all
api/series-traverse/find_and
api/series-traverse/find_btw
api/series-traverse/find_eq
api/series-traverse/find_first
api/series-traverse/find_ge
api/series-traverse/find_if
api/series-traverse/find_le
api/series-traverse/find_neq
api/series-traverse/find_not
api/series-traverse/find_or
api/series-traverse/match
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/traverse.html">
    <span class="label">Previous</span>
    <span class="title">traverse</span>
  </a>
  <a class="next-link" href="api/series-traverse/find_all.html">
    <span class="label">Next</span>
    <span class="title">find_all</span>
  </a>
</div>
