<!-- pluss-hand-authored -->

# Series::repeat_interleave

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::repeat_interleave(std::int64_t repeats) const
Series Series::repeat_interleave(std::int64_t repeats, std::int64_t dim) const
```

## What It Does

`repeat_interleave()` repeats each element `repeats` times **in place**, so
`[10, 20, 30]` with `repeats = 2` becomes `[10, 10, 20, 20, 30, 30]`.

This is the counterpart to [`repeat()`](repeat.md): `repeat()` tiles the whole
block (`ABCABC`), `repeat_interleave()` expands each element next to itself
(`AABBCC`).

- **Without `dim`**, the series is flattened to 1-D first, then every element is
  interleaved.
- **With `dim`**, each slice along that axis is repeated `repeats` times,
  growing that axis by the factor `repeats` and leaving the shape otherwise
  intact.

The result is a fresh materialized array.

## Parameters

**repeats** (`std::int64_t`)

How many consecutive copies of each element (or each slice, with `dim`).

**dim** (`std::int64_t`, optional)

The axis along which to interleave. Omitted: flatten first, then interleave.

## Returns

`Series`
    A contiguous array. With `dim`, axis `dim` has length `size(dim) *
    repeats`; without it, the result is 1-D of length `numel() * repeats`.

## Gradient

`repeat_interleave()` is differentiable. Each source element (or slice) feeds
`repeats` consecutive outputs, so the backward sums the upstream gradient over
those `repeats` positions.

## Examples

Example 1. Each element doubled in place.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({10.0, 20.0, 30.0}).repeat_interleave(2));
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[10.0000, 10.0000, 20.0000, 20.0000, 30.0000, 30.0000]
```

Example 2. `dim = 0` repeats each row; `dim = 1` repeats each column.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

std::cout << "dim 0:\n" << m.repeat_interleave(2, 0);
std::cout << "\ndim 1:\n" << m.repeat_interleave(2, 1);
```

### Output

```text
dim 0:
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [1, 2],
  [3, 4],
  [3, 4]]
dim 1:
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 1, 2, 2],
  [3, 3, 4, 4]]
```

Example 3. The backward sums each element's gradient over its consecutive
copies.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

x.repeat_interleave(2).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

Example 4. With per-copy weights the two copies of each element accumulate
separately.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto weights = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0});
(x.repeat_interleave(2) * weights).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[30, 70, 110]
```

Element `0` fed outputs `0` and `1`, so it collects `10 + 20 = 30`; element `1`
collects `30 + 40 = 70`; element `2` collects `50 + 60 = 110`.

## Gotchas And Failure Modes

- Without `dim` the series is flattened first, so a 2-D input comes back 1-D.
  Pass `dim` to keep the rank.
- The ordering is `AABBCC`, not `ABCABC` — for the latter use
  [`repeat()`](repeat.md).
- The result is a copy, not a view.

## Summary

`repeat_interleave(repeats [, dim])` expands each element in place:

- `AABBCC` ordering; grows one axis (or flattens, then the whole length)
- always contiguous
- differentiable, backward sums each source element's gradient over its
  `repeats` consecutive copies

## See Also

- [`Series::repeat`](repeat.md): tiles the whole block instead (`ABCABC`).
- [`Series::expand`](expand.md): a zero-copy broadcast of size-`1` axes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeat.html">
    <span class="label">Previous</span>
    <span class="title">repeat</span>
  </a>
  <a class="next-link" href="reshape.html">
    <span class="label">Next</span>
    <span class="title">reshape</span>
  </a>
</div>
