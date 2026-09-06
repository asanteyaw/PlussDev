<!-- pluss-hand-authored -->

# Series::traverse

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
SeriesTraverseAccessor Series::traverse;

SeriesTraverseProxy Series::traverse() const;
SeriesTraverseProxy Series::traverse(std::initializer_list<ExtractSpec> on_specs) const;
SeriesTraverseProxy Series::traverse(std::span<const ExtractSpec> on_specs) const;
```

Every finder method (`find_ge`, `find_eq`, `find_all`, `find_first`, ...) is
reachable both directly on the accessor and on the proxy returned by
`traverse(...)`. Each finder returns a [`TraverseMatch`](../series-traverse/match.md).

## What It Does

`traverse` is a **content search** over a `Series`. You describe a condition on
the values, and it returns the *coordinates* of every position that satisfies
it — together with the matched values, the flat offsets, and helpers to pull
the same positions out of another `Series`.

This is a place where `Series` deliberately steps outside the tensor-library
mould. The usual tools answer "where?" only indirectly:

- a comparison such as `x.ge(5.0)` returns a boolean `Series` the same shape as
  `x` — you still have to turn that mask into positions yourself
- `x.nonzero()` turns a mask into a `[K, rank]` coordinate block — but you have
  to build and pass the mask, and you get back only raw coordinates

`traverse` folds that whole pipeline into one call and hands back a small
**result object** you can query several ways:

```cpp
auto hits = temps.traverse.find_ge(30.0);

hits.count();            // how many positions matched
hits.coords();           // [K, rank] int64 grid coordinates, row-major order
hits.values();           // [K] the matched values (differentiable)
hits.linear_indices();   // [K] flat row-major offsets
hits.split_indices();    // rank separate [K] index columns
hits.select_from(other); // gather those same positions out of `other`
```

Reach for `traverse` whenever the question is "*which elements* satisfy this,
and where are they" rather than "reshape / reduce / transform every element."

## The Accessor And The Proxy

`traverse` on its own searches the **whole** `Series`:

```cpp
auto hits = grid.traverse.find_ge(6.0);
```

`traverse({ ...selector... })` first narrows to a sub-view and searches only
that:

```cpp
auto row1_hits = grid.traverse({std::int64_t{1}}).find_ge(5.0);   // row 1 only
auto last_col  = grid.traverse({pluss::Slicing::Ellipsis(),
                                std::int64_t{2}}).find_ge(6.0);    // last column
```

The selector list is the same view-only language accepted by
[`traverse_slice()`](traverse_slice.md) — scalar indices, positive-step slices,
`SliceAll()`, `NewAxis()`, `Ellipsis()`. **Coordinates in the result are
relative to the traversed view**, not to the original `Series`: after
`traverse({1})` on a `3 x 3`, a `TraverseMatch` has `rank() == 1` and its
coordinates index the three columns of row `1`.

## Finder Families

| Family | Methods | Page |
|---|---|---|
| Explicit mask | `find_all`, `find_if` | [`find_all`](../series-traverse/find_all.md) |
| Equality | `find_eq`, `find_neq` | [`find_eq`](../series-traverse/find_eq.md) |
| Threshold | `find_ge`, `find_le` | [`find_ge`](../series-traverse/find_ge.md) |
| Range | `find_btw` | [`find_btw`](../series-traverse/find_btw.md) |
| Mask algebra | `find_and`, `find_or`, `find_not` | [`find_and`](../series-traverse/find_and.md) |
| First-per-lane | `find_first` | [`find_first`](../series-traverse/find_first.md) |

Every one produces a [`TraverseMatch`](../series-traverse/match.md); that page
documents the result object.

Open the [SeriesTraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)
for the full method index.

## Returns

`SeriesTraverseAccessor` (the member) / `SeriesTraverseProxy` (from calling it)
    A lightweight search entrypoint bound to the owning `Series`. Reading
    `traverse` copies no data and computes nothing until a finder runs.

## Gradient

The search itself — coordinates, counts, flat offsets — is pure integer
bookkeeping and carries no gradient. But the two operations that produce
*values* are differentiable:

- `match.values()` is a gather from the searched `Series`; its backward routes
  sensitivity to exactly the matched positions.
- `match.select_from(other)` is a gather from `other`; its backward routes
  sensitivity into `other` at the matched positions.

So a `traverse` result can sit inside a training graph as long as gradient
flows through `values()` / `select_from()` and not through the coordinates.

## Example

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto hits = grid.traverse.find_ge(6.0);

std::cout << "count=" << hits.count() << "\n";
std::cout << "coords=" << hits.coords();
std::cout << "values=" << hits.values();
```

### Output

```text
count=5
coords=pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[0, 0],
  [0, 2],
  [1, 1],
  [1, 2],
  [2, 0]]
values=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10, 7, 9, 6, 8]
```

Coordinates come back in row-major order, so `values` reads
`[grid[0,0], grid[0,2], grid[1,1], grid[1,2], grid[2,0]]`.

## Gotchas And Failure Modes

- A finder always returns a defined `TraverseMatch`. "No matches" is
  `count() == 0` with empty `coords()` / `values()`, never an undefined result
  or an exception.
- With `traverse({ on })`, result coordinates are relative to the **sub-view**,
  and `rank()` is the sub-view's rank.
- `find_all` / `find_if` require the mask shape to match the traversed
  shape exactly — no broadcasting.
- Coordinates and counts are non-differentiable; only `values()` and
  `select_from()` carry gradient.
- Match order is row-major (last axis fastest), the same order
  [`nonzero()`](nonzero.md) uses.

## Summary

`traverse` is the value-driven search surface:

- one call from "condition" to "coordinates + matched values + flat offsets"
- `traverse` searches the whole `Series`; `traverse({on})` searches a sub-view
- results are a queryable [`TraverseMatch`](../series-traverse/match.md), not a
  bare mask
- `values()` and `select_from()` are differentiable; the index data is not

## See Also

- [SeriesTraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md):
  the full finder index.
- [`TraverseMatch`](../series-traverse/match.md): the result object every finder
  returns.
- [`Series::nonzero`](nonzero.md): the lower-level mask-to-coordinates step
  `traverse` builds on.
- [`Series::traverse_slice`](traverse_slice.md): the view-only selector language
  reused by `traverse({ on })`.
- [`Series::extract`](extract.md): materializing selection when you already know
  the indices.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
  <a class="next-link" href="../../series_traverse_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">SeriesTraverseAccessor</span>
  </a>
</div>
