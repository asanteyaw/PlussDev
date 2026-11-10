<!-- pluss-hand-authored -->

# TraverseMatch

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Type

Every finder on [`Series::traverse`](../series/traverse.md) —
[`find_ge`](find_ge.md), [`find_eq`](find_eq.md), [`find_all`](find_all.md),
[`find_first`](find_first.md), and the rest — returns a `TraverseMatch`. It is
the small result object that holds a search's hits and lets you read them
several ways.

## Members

```cpp
bool                        defined() const noexcept;
std::int64_t                count() const;
std::int64_t                rank() const noexcept;
std::int64_t                indexed_rank() const noexcept;   // alias of rank()
std::span<const std::int64_t> source_shape() const noexcept;
bool                        has_values() const noexcept;

const Series&               coords() const noexcept;         // [K, rank] int64
const Series&               values() const noexcept;         // [K] matched values
Series                      linear_indices() const;          // [K] flat offsets
pluss::StackArray<Series>   split_indices() const;           // rank * [K] columns
Series                      select_from(const Series& other) const;
```

## What Each Member Gives You

**`count()`** — the number of matched positions, `K`. Zero is normal and means
"nothing matched"; it is never an error.

**`defined()`** — whether this is a real match object. A finder always returns a
defined match (even with `count() == 0`); only a default-constructed
`TraverseMatch{}` is undefined.

**`rank()`** / **`indexed_rank()`** — the number of axes each coordinate has.
For a whole-`Series` search this is the source rank; for a
`traverse({ on })` search it is the **sub-view's** rank. The two spellings are
identical.

**`source_shape()`** — the shape the coordinates index into (the traversed
shape, which is the sub-view's shape under `traverse({ on })`).

**`has_values()`** — whether `values()` is populated. The finders all populate
it; it is `false` only for a match built from coordinates alone.

**`coords()`** — a `[K, rank]` `int64` `Series`. Row `i` is the full coordinate
of the `i`-th match, in row-major order (last axis fastest).

**`values()`** — a `[K]` `Series` of the matched values, gathered from the
traversed `Series` in the same order as `coords()`. This gather **is
differentiable**: its backward routes sensitivity to the matched positions of
the traversed `Series`.

**`linear_indices()`** — a `[K]` `int64` `Series` of flat row-major offsets,
i.e. each coordinate folded through the source strides. Handy for feeding
[`take()`](../series/take.md) or comparing membership.

**`split_indices()`** — a `pluss::StackArray<Series>` of `rank` separate `[K]`
`int64` columns: element `d` is the `d`-th coordinate of every match. This is
the layout [`extract()`](../series/extract.md) and advanced indexing expect.

**`select_from(other)`** — gather the matched positions out of a *different*
`Series`. `other` must agree with `source_shape()` on the indexed axes; any
trailing axes are carried through. This gather **is differentiable** with
respect to `other`.

## Examples

Example 1. The full set of views on one search.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto m = grid.traverse.find_ge(6.0);

std::cout << "count=" << m.count() << "  rank=" << m.rank() << "\n";
std::cout << "coords=" << m.coords();
std::cout << "values=" << m.values();
std::cout << "linear=" << m.linear_indices();

auto cols = m.split_indices();
std::cout << "rows=" << cols[0];
std::cout << "cols=" << cols[1];
```

### Output

```text
count=5  rank=2
coords=pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[0, 0],
  [0, 2],
  [1, 1],
  [1, 2],
  [2, 0]]
values=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10, 7, 9, 6, 8]
linear=pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 2, 4, 5, 6]
rows=pluss::Series(shape=[5], strides=[2], dtype=int64, platform=cpu, numel=5, contiguous=false)
data=[0, 0, 1, 1, 2]
cols=pluss::Series(shape=[5], strides=[2], dtype=int64, platform=cpu, numel=5, contiguous=false)
data=[0, 2, 1, 2, 0]
```

`coords()` row `[1, 2]` is the same position as `linear_indices()` entry `5`
(row `1` times three columns plus column `2`), and the same as `rows[3] = 1`,
`cols[3] = 2`.

Example 2. `select_from` pulls the matched positions out of a parallel `Series`.

```cpp
auto scores = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});
auto labels = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});

auto top = scores.traverse.find_ge(8.0);

std::cout << "matched scores=" << top.values();
std::cout << "labels at those spots=" << top.select_from(labels);
```

### Output

```text
matched scores=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 9, 8]
labels at those spots=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 5, 7]
```

`scores >= 8` at `[0,0]`, `[1,1]`, `[2,0]`, so `select_from(labels)` returns
`labels` at exactly those three positions.

Example 3. `select_from` is differentiable with respect to its argument.

```cpp
auto scores = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});
auto weights = pluss::make_series({
    {1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
});
weights.tracks_slope_(true);

scores.traverse.find_ge(8.0).select_from(weights).sum().reverse();

std::cout << weights.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [1, 0, 0]]
```

Only the gathered positions of `weights` receive gradient.

Example 4. Empty and default matches.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto none = grid.traverse.find_ge(999.0);
std::cout << "none: defined=" << (none.defined() ? "yes" : "no")
          << " count=" << none.count() << "\n";

pluss::codem::TraverseMatch blank;
std::cout << "blank: defined=" << (blank.defined() ? "yes" : "no")
          << " count=" << blank.count() << "\n";
```

### Output

```text
none: defined=yes count=0
blank: defined=no count=0
```

A finder that matched nothing is still a defined match with empty `coords()`
and `values()`. Only a hand-constructed `TraverseMatch{}` is undefined.

## Gotchas And Failure Modes

- `coords()` and `values()` return references into the match; keep the
  `TraverseMatch` alive while you use them.
- `split_indices()` columns are stride views over `coords()` (non-contiguous);
  call [`contiguous()`](../series/contiguous.md) if a consumer needs a dense
  buffer.
- Under `traverse({ on })`, `rank()` and `source_shape()` describe the
  sub-view, and `coords()` index the sub-view — not the original `Series`.
- `select_from(other)` checks `other` against `source_shape()` on the indexed
  axes and raises on a mismatch.
- `values()` and `select_from()` carry gradient; `coords()`,
  `linear_indices()`, and `split_indices()` do not.

## Summary

`TraverseMatch` is the queryable result of every `traverse` finder:

- `count()` / `defined()` — how many, and whether it is a real match
- `coords()` / `linear_indices()` / `split_indices()` — the positions, three
  layouts
- `values()` — the matched values (differentiable)
- `select_from(other)` — the same positions gathered from another `Series`
  (differentiable)
- `rank()` / `source_shape()` — the coordinate space, sub-view-relative under
  `traverse({ on })`

## See Also

- [`Series::traverse`](../series/traverse.md): the accessor that produces a
  `TraverseMatch`.
- [`Series::nonzero`](../series/nonzero.md): the raw mask-to-coordinates step.
- [`Series::extract`](../series/extract.md): consumes `split_indices()`-style
  index columns.
- [`Series::take`](../series/take.md): consumes `linear_indices()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_or.html">
    <span class="label">Previous</span>
    <span class="title">find_or</span>
  </a>
  <a class="next-link" href="../../series_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series API Reference</span>
  </a>
</div>
