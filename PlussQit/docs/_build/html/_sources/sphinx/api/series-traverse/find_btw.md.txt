<!-- pluss-hand-authored -->

# SeriesTraverseAccessor::find_btw

[Back to SeriesTraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch SeriesTraverseAccessor::find_btw(double lo, double hi,
                                               bool inclusive = true) const;
TraverseMatch SeriesTraverseAccessor::find_btw(const Series& lo, const Series& hi,
                                               bool inclusive = true) const;

TraverseMatch SeriesTraverseProxy::find_btw(double lo, double hi,
                                            bool inclusive = true) const;
TraverseMatch SeriesTraverseProxy::find_btw(const Series& lo, const Series& hi,
                                            bool inclusive = true) const;
```

## What It Does

`find_btw(lo, hi)` returns the coordinates of every position whose value falls
inside the band between `lo` and `hi`. It is [`find_all`](find_all.md) with a
two-sided mask built for you:

- `inclusive = true` (default): `value >= lo && value <= hi`
- `inclusive = false`: `value > lo && value < hi`

Both bounds accept either a scalar or a broadcast peer `Series`, so the band can
vary by row, column, or position — for example "values within one standard
deviation of each row's mean."

`find_btw` is also the practical way to match a *computed* value with tolerance:
comparisons produced by arithmetic rarely satisfy exact
[`find_eq`](find_eq.md), but a narrow `find_btw(target - eps, target + eps)`
band does.

## Parameters

**lo** (`double` or `const Series&`)

The lower bound.

**hi** (`double` or `const Series&`)

The upper bound.

**inclusive** (`bool`, default `true`)

`true` includes the endpoints; `false` excludes both. There is no one-sided
option — for that use [`find_ge`](find_ge.md) or [`find_le`](find_le.md).

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the in-band positions. `K` can
    be `0`.

## Gradient

Coordinates carry no gradient. `match.values()` and `match.select_from(other)`
are differentiable gathers.

## Examples

Example 1. An inclusive band keeps both endpoints.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto band = grid.traverse.find_btw(4.0, 8.0);

std::cout << "count=" << band.count() << "\n";
std::cout << band.coords();
std::cout << band.values();
```

### Output

```text
count=5
pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[0, 1],
  [0, 2],
  [1, 2],
  [2, 0],
  [2, 2]]
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[4, 7, 6, 8, 5]
```

The endpoints `4` (row `0`, column `1`) and `8` (row `2`, column `0`) are both
in the result.

Example 2. The same band, exclusive, drops the endpoints.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto strict = grid.traverse.find_btw(4.0, 8.0, /*inclusive=*/false);

std::cout << "count=" << strict.count() << "\n";
std::cout << strict.values();
```

### Output

```text
count=3
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[7, 6, 5]
```

`4` and `8` are gone; only `7`, `6`, `5` remain strictly between.

Example 3. Tolerance matching for a computed value.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto approx_seven = grid.traverse.find_btw(7.0 - 1e-9, 7.0 + 1e-9);

std::cout << "count=" << approx_seven.count() << "\n";
std::cout << approx_seven.coords();
```

### Output

```text
count=1
pluss::Series(shape=[1,2], strides=[2,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[0, 2]]
```

## Gotchas And Failure Modes

- `inclusive` toggles both endpoints together; there is no per-side control.
- For a one-sided bound use [`find_ge`](find_ge.md) or [`find_le`](find_le.md).
- `Series` bounds must broadcast to the traversed shape.
- An empty band (`lo > hi`) simply produces `count() == 0`.
- Coordinates are non-differentiable; `values()` and `select_from()` are.

## Summary

`find_btw(lo, hi [, inclusive])` finds every position inside a band:

- `inclusive` default keeps endpoints; pass `false` to exclude both
- scalar or broadcast-`Series` bounds
- the tolerance-friendly alternative to [`find_eq`](find_eq.md)

## See Also

- [`find_ge`](find_ge.md) / [`find_le`](find_le.md): one-sided bounds.
- [`find_eq`](find_eq.md): exact match; `find_btw` with a narrow band adds
  tolerance.
- [`find_all`](find_all.md): supply an arbitrary two-sided mask.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_and.html">
    <span class="label">Previous</span>
    <span class="title">find_and</span>
  </a>
  <a class="next-link" href="find_eq.html">
    <span class="label">Next</span>
    <span class="title">find_eq</span>
  </a>
</div>
