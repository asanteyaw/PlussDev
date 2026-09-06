<!-- pluss-hand-authored -->

# Series::tile

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tile(pluss::RangeView::Int reps) const -> Series
```

## What It Does

`tile()` is an alias for [`repeat()`](repeat.md): it tiles the whole series
`reps[i]` times along axis `i` and returns a new, materialized (copied) array.

`[7, 8]` tiled `{3}` is `[7, 8, 7, 8, 7, 8]` — the block repeated, not the
individual elements.

Both names route to the same implementation, so anything in the
[`repeat()`](repeat.md) reference applies here unchanged: the shape rule
(`size(i) * reps[i]`), the fact that it copies rather than viewing, and the
differentiable backward that sums each source element's gradient over its
copies.

## Parameters

**reps** (`pluss::RangeView::Int`)

One repeat count per axis; extra leading entries add new tiled dimensions.

## Returns

`Series`
    A fresh contiguous array with axis `i` of length `size(i) * reps[i]`.

## Gradient

Differentiable, identical to [`repeat()`](repeat.md): each source element
accumulates the gradient of every copy it produced.

## Examples

Example 1. Tiling a row.

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

Example 2. The backward, tiled three times.

```cpp
auto x = pluss::make_series({7.0, 8.0});
x.tracks_slope_(true);

x.tile({3}).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3, 3]
```

Each element appears in three tiles, so it accumulates `3`.

## Gotchas And Failure Modes

- `tile()` copies; for a zero-copy stretch use [`expand()`](expand.md).
- It tiles the block. To repeat each element in place, use
  [`repeat_interleave()`](repeat_interleave.md).

## Summary

`tile(reps)` is [`repeat()`](repeat.md) under a second name — block tiling with a
materialized, differentiable result.

## See Also

- [`Series::repeat`](repeat.md): the primary name and full reference.
- [`Series::repeat_interleave`](repeat_interleave.md): element-wise repetition.
- [`Series::expand`](expand.md): zero-copy broadcast.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tensor_split.html">
    <span class="label">Previous</span>
    <span class="title">tensor_split</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
