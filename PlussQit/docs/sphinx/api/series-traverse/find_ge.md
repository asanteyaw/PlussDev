<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_ge

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_ge(double rhs) const;
TraverseMatch Series::TraverseAccessor::find_ge(const Series& rhs) const;

TraverseMatch SeriesTraverseProxy::find_ge(double rhs) const;
TraverseMatch SeriesTraverseProxy::find_ge(const Series& rhs) const;
```

The mirror method [`find_le`](find_le.md) has the same shape and returns
positions **at or below** the threshold.

## What It Does

`find_ge(rhs)` returns the coordinates of every position whose value is greater
than or equal to `rhs`. It is [`find_all`](find_all.md) with the mask
`source.ge(rhs)` built for you.

- **`find_ge(double)`** uses one scalar threshold for the whole `Series`.
- **`find_ge(const Series&)`** uses a broadcast peer, so the threshold can vary
  by row, column, or position — useful for "which values exceed this row's own
  mean," and similar per-lane tests.

The comparison is inclusive (`>=`). For a strict `>` build the mask yourself and
call [`find_all`](find_all.md), or use [`find_btw`](find_btw.md) with the
exclusive flag.

## Parameters

**rhs** (`double` or `const Series&`)

The lower bound. A `Series` `rhs` broadcasts against the traversed shape.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the positions at or above the
    bound. `K` can be `0`.

## Gradient

Coordinates carry no gradient. `match.values()` and `match.select_from(other)`
are differentiable gathers.

## Examples

Example 1. A scalar threshold, and the several views the result offers.

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
std::cout << "linear=" << hits.linear_indices();
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
linear=pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 2, 4, 5, 6]
```

`linear_indices()` are the flat row-major offsets: `grid[0,0]` is offset `0`,
`grid[1,2]` is offset `5`.

Example 2. A per-column threshold with a `Series` right-hand side.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto floor = pluss::make_series({5.0, 5.0, 5.0});
std::cout << "count=" << grid.traverse.find_ge(floor).count();
```

### Output

```text
count=6
```

Example 3. Restrict the search to one row with the proxy form.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto row1 = grid.traverse({std::int64_t{1}}).find_ge(5.0);

std::cout << "count=" << row1.count() << "  rank=" << row1.rank() << "\n";
std::cout << row1.coords();
std::cout << row1.values();
```

### Output

```text
count=2  rank=1
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[9, 6]
```

The sub-view is one-dimensional, so `rank()` is `1` and the coordinates index
the three columns of row `1` — position `1` (`9`) and position `2` (`6`).

Example 4. `values()` participates in a training graph.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});
grid.tracks_slope_(true);

grid.traverse.find_ge(6.0).values().sum().reverse();

std::cout << grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 1],
  [0, 1, 1],
  [1, 0, 0]]
```

## Gotchas And Failure Modes

- The bound is inclusive. Use [`find_all`](find_all.md) with `source.gt(rhs)`
  for a strict comparison.
- With `traverse({ on })`, coordinates are relative to the sub-view and
  `rank()` is the sub-view's rank.
- A `Series` `rhs` must broadcast to the traversed shape.
- Coordinates and offsets are non-differentiable; `values()` and
  `select_from()` are.

## Summary

`find_ge(rhs)` finds every position at or above a scalar or broadcast bound:

- convenience over [`find_all`](find_all.md) with a `ge` mask
- [`find_le`](find_le.md) is the at-or-below mirror
- inclusive comparison; empty match when nothing qualifies

## See Also

- [`find_le`](find_le.md): the at-or-below mirror.
- [`find_btw`](find_btw.md): a two-sided band, inclusive or exclusive.
- [`find_all`](find_all.md): supply a strict `>` mask yourself.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_first.html">
    <span class="label">Previous</span>
    <span class="title">find_first</span>
  </a>
  <a class="next-link" href="find_if.html">
    <span class="label">Next</span>
    <span class="title">find_if</span>
  </a>
</div>
