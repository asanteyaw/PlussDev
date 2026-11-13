<!-- pluss-hand-authored -->

# Series::select

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::select(std::int64_t dim, std::int64_t index) const
```

## What It Does

`select()` picks one position along one axis and removes that axis from the
result.

That makes it one of the simplest read-side indexing operations on the
surface:

- choose an axis
- choose one index on that axis
- get back the corresponding slice

Unlike [`slice()`](slice.md), this is not a range operation. Unlike
[`index_select()`](index_select.md), this does not keep the selected axis with
length greater than one. It is the single-position, rank-reducing form.

`select()` is also view-like rather than materializing fresh data. The result
points at the chosen lane inside the original storage, so it preserves layout
properties such as non-contiguous strides when the chosen view is strided.

## Parameters

**dim** (`std::int64_t`)

The axis to index into.

- positive dims count from the front
- negative dims count from the back
- the normalized dim must refer to a real axis

This toggle changes which slice family you are selecting from. On a `3 x 3`
series:

- `dim = 0` chooses one row
- `dim = 1` or `dim = -1` chooses one column

**index** (`std::int64_t`)

The position to pick along `dim`.

- non-negative indices count from the front
- negative indices count from the back
- the normalized index must land inside the chosen axis

This parameter is exact: one position in, one slice out.

## Returns

`Series`
    A view of the chosen slice, with `dim` removed from the shape.

If `self` has rank `n`, the result has rank `n - 1`.

## Gradient

`select()` is differentiable in `self`.

Backward expands the outgoing sensitivity back along the removed axis and
routes it only into the chosen position. Every non-selected lane receives zero
gradient.

That makes `select()` useful in real model code when you need one row, one
column, one timestep, or one feature lane and still want correct reverse-mode
behavior.

## Examples

Example 1. Selecting along `dim = 0` from a `3 x 3` series returns one row.

```cpp
auto grid = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
    {70.0, 80.0, 90.0},
});

pluss::series::print(std::cout, grid.select(0, 1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[40.0000, 50.0000, 60.0000]
```

Axis `0` disappeared, so the result is rank `1`.

Example 2. Negative dims let you spell "last axis" directly.

```cpp
std::cout << grid.select(-1, 2);
```

### Output

```text
pluss::Series(shape=[3], strides=[3], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[30, 60, 90]
```

This selected the last column. Notice the stride is `3`, so the result is a
strided view rather than a fresh contiguous copy.

Example 3. Negative indices are normalized from the back of the chosen axis.

```cpp
std::cout << grid.select(1, -1);
```

### Output

```text
pluss::Series(shape=[3], strides=[3], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[30, 60, 90]
```

For axis `1`, index `-1` means "last column", so this matches the previous
example.

Example 4. Reverse-mode only touches the selected lane.

```cpp
auto tracked = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
tracked.tracks_slope_(true);

auto selected = tracked.select(0, 1);
selected.sum().reverse();

std::cout << "out=" << selected << "\n";
std::cout << "dx=" << tracked.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 6]
dx=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[0, 0, 0],
  [1, 1, 1],
  [0, 0, 0]]
```

Only row `1` participated in the forward result, so only row `1` receives
gradient in backward.

Example 5. Rank-0 series cannot be selected along an axis because they do not
have one.

```cpp
try {
  std::cout << pluss::make_series(42.0).select(0, 0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
select: cannot be applied to a 0-dim Series
```

Example 6. Out-of-range indices fail explicitly.

```cpp
try {
  std::cout << grid.select(0, 5) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
select: index out of range
```

## Gotchas And Failure Modes

- `select()` removes the chosen axis. If you need to keep that axis with
  length `1`, use [`slice()`](slice.md) instead.
- The result can be non-contiguous. That is normal for column-style selections
  from a row-major layout.
- Negative dims and indices are convenient, but they still must normalize to a
  real axis and a real position.
- Scalar series have no selectable axis, so `select()` is invalid on rank `0`.

## Summary

Use `select()` when you want exactly one lane and you want rank to drop.

- choose `dim` to say which axis you are indexing
- choose `index` to say which position on that axis you want
- expect a view-like result, not a copied series
- expect backward to route gradient only to the chosen lane

## See Also

- [`Series::slice`](slice.md): keep a range instead of one position.
- [`Series::index_select`](index_select.md): keep several positions and keep
  the selected axis.
- [`Series::take`](take.md): flat-position gather rather than axis-aware
  selection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_reduce_sum.html">
    <span class="label">Previous</span>
    <span class="title">scatter_reduce_sum</span>
  </a>
  <a class="next-link" href="sensitivity.html">
    <span class="label">Next</span>
    <span class="title">sensitivity</span>
  </a>
</div>
