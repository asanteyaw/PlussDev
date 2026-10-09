<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_all

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_all(const Series& mask) const;
TraverseMatch Series::TraverseAccessor::find_if(const Series& mask) const;

TraverseMatch SeriesTraverseProxy::find_all(const Series& mask) const;
TraverseMatch SeriesTraverseProxy::find_if(const Series& mask) const;
```

`find_if` is an identical spelling of `find_all`; see
[`find_if`](find_if.md).

## What It Does

`find_all(mask)` is the base finder that every other finder is built on. You
supply a boolean `Series` the same shape as the traversed `Series`, and it
returns a [`TraverseMatch`](match.md) holding the coordinates of every `true`
position, in row-major order.

The comparison finders — [`find_eq`](find_eq.md), [`find_ge`](find_ge.md),
[`find_btw`](find_btw.md), and so on — are conveniences that build the mask for
you and then call `find_all`. Use `find_all` directly when the condition is not
a single comparison: a parity test, a result of several `logical_*` steps, an
externally computed mask, and so on.

The mask is interpreted as "non-zero means keep." A non-boolean `Series` is
accepted and compared against zero; a boolean `Series` is used as-is.

## Parameters

**mask** (`const Series&`)

A `Series` whose shape **exactly matches** the traversed shape. No broadcasting
is applied — a shape mismatch is an error, not a stretch. If the accessor was
opened with `traverse({ on })`, the mask must match the sub-view's shape.

## Returns

[`TraverseMatch`](match.md)
    A defined match. `coords()` is `[K, rank]` `int64`; `values()` is the `[K]`
    matched values gathered from the traversed `Series`. `K` can be `0`.

## Gradient

The coordinates carry no gradient. `match.values()` is a differentiable gather
from the traversed `Series`, and `match.select_from(other)` is a differentiable
gather from `other`.

## Examples

Example 1. Every position above a computed threshold.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto hits = grid.traverse.find_all(grid.gt(5.0));

std::cout << "count=" << hits.count() << "\n";
std::cout << hits.coords();
```

### Output

```text
count=5
pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[0, 0],
  [0, 2],
  [1, 1],
  [1, 2],
  [2, 0]]
```

Example 2. An arbitrary mask you built elsewhere — the comparison finders do
not have to be involved at all.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto keep = pluss::make_series({
    {true,  false, true},
    {false, true,  false},
    {true,  false, false},
});
auto hits = grid.traverse.find_all(keep);

std::cout << "count=" << hits.count() << "\n";
std::cout << "values=" << hits.values();
```

### Output

```text
count=4
values=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 7, 9, 8]
```

Example 3. A non-boolean mask is read as "non-zero means keep."

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto counts = pluss::make_series({
    {0.0, 3.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, 0.0, 2.0},
});

std::cout << "count=" << grid.traverse.find_all(counts).count();
```

### Output

```text
count=3
```

Example 4. No matches is a clean empty result, not an error.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto none = grid.traverse.find_all(grid.gt(1000.0));

std::cout << "defined=" << (none.defined() ? "yes" : "no") << "\n";
std::cout << "count=" << none.count() << "\n";
std::cout << none.coords();
```

### Output

```text
defined=yes
count=0
pluss::Series(shape=[0,2], strides=[2,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 5. The mask shape must match exactly — no broadcasting.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

try {
  auto row_mask = pluss::make_series({true, false, true}, {3});
  grid.traverse.find_all(row_mask);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse: mask shape must exactly match the queried Series/view shape
```

Example 6. `values()` feeds a differentiable graph.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});
grid.tracks_slope_(true);

grid.traverse.find_all(grid.ge(6.0)).values().sum().reverse();

std::cout << grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 1],
  [0, 1, 1],
  [1, 0, 0]]
```

Only the matched positions receive gradient.

## Gotchas And Failure Modes

- The mask must match the traversed shape exactly; there is no broadcasting.
- `find_all` and `find_if` are the same operation.
- Match order is row-major (last axis fastest).
- Coordinates and `linear_indices()` are non-differentiable; `values()` and
  `select_from()` are.

## Summary

`find_all(mask)` returns the coordinates of every true position as a
[`TraverseMatch`](match.md):

- the base finder — the comparison finders build a mask and call it
- exact shape match required, "non-zero means keep"
- empty match on zero hits, never an error

## See Also

- [`find_eq`](find_eq.md), [`find_ge`](find_ge.md), [`find_btw`](find_btw.md):
  comparison finders that build the mask for you.
- [`find_and`](find_and.md): combine two masks without a temporary.
- [`TraverseMatch`](match.md): the result object.
- [`Series::nonzero`](../series/nonzero.md): the coordinate primitive underneath.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_traverse_accessor_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Series::TraverseAccessor</span>
  </a>
  <a class="next-link" href="find_and.html">
    <span class="label">Next</span>
    <span class="title">find_and</span>
  </a>
</div>
