<!-- pluss-hand-authored -->

# Series::repeat

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::repeat(pluss::RangeView::Int reps) const -> Series
```

## What It Does

`repeat()` tiles the whole series `reps[i]` times along axis `i` and returns a
new, **materialized** array — unlike [`expand()`](expand.md), this one copies.

For a length-2 row `[7, 8]` and `reps = {3}`, the result is
`[7, 8, 7, 8, 7, 8]` — the entire block laid end to end three times. With a
2-D `reps` the tiling happens per axis: `reps = {2, 3}` on a `2 x 2` matrix
gives a `4 x 6` result.

The key contrast with [`repeat_interleave()`](repeat_interleave.md): `repeat()`
tiles the *block* (`ABAB...`), while `repeat_interleave()` repeats each *element*
in place (`AABB...`).

`tile()` is an alias for `repeat()`.

## Parameters

**reps** (`pluss::RangeView::Int`)

One repeat count per axis. Must have at least the current rank; extra leading
entries add new tiled dimensions.

## Returns

`Series`
    A fresh contiguous array whose axis `i` has length `size(i) * reps[i]`.

## Gradient

`repeat()` is differentiable. Since each source element appears in the output
`prod(reps)` times (once per tile), the backward sums the upstream gradient over
all the tiled copies — every source element accumulates the gradient of each of
its copies.

## Examples

Example 1. Tiling a row three times.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({7.0, 8.0}).repeat({3}));
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[7.0000, 8.0000, 7.0000, 8.0000, 7.0000, 8.0000]
```

Example 2. Per-axis repeat counts on a matrix.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

std::cout << m.repeat({2, 2});
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[1, 2, 1, 2],
  [3, 4, 3, 4],
  [1, 2, 1, 2],
  [3, 4, 3, 4]]
```

The `2 x 2` block is tiled twice down and twice across.

Example 3. `tile()` is the same call.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({7.0, 8.0}).tile({3}));
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[7.0000, 8.0000, 7.0000, 8.0000, 7.0000, 8.0000]
```

Example 4. The backward sums each source element's gradient over all its
copies. Here `reps = {2, 2}` makes four copies of every element.

```cpp
auto x = pluss::make_series({7.0, 8.0});
x.tracks_slope_(true);

x.repeat({2, 2}).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 4]
```

## Gotchas And Failure Modes

- `repeat()` copies. For a zero-copy stretch of size-`1` axes, use
  [`expand()`](expand.md) instead.
- `reps` is per-axis repeat *counts*, not a target shape. `reps = {1, 1}` is a
  plain contiguous copy.
- `repeat()` tiles the block; [`repeat_interleave()`](repeat_interleave.md)
  repeats each element in place. They give different orderings.

## Summary

`repeat(reps)` tiles the whole series and materializes the result:

- axis `i` becomes `size(i) * reps[i]`; extra leading entries add tiled axes
- always contiguous; `tile()` is an alias
- differentiable, backward sums each source element's gradient over its
  `prod(reps)` copies

## See Also

- [`Series::tile`](tile.md): the alias.
- [`Series::repeat_interleave`](repeat_interleave.md): repeat each element in
  place rather than tiling the block.
- [`Series::expand`](expand.md): a zero-copy broadcast instead of a copy.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remainder.html">
    <span class="label">Previous</span>
    <span class="title">remainder</span>
  </a>
  <a class="next-link" href="repeat_interleave.html">
    <span class="label">Next</span>
    <span class="title">repeat_interleave</span>
  </a>
</div>
