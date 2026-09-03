<!-- pluss-hand-authored -->

# Series::unfold

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unfold(std::int64_t dim, std::int64_t size, std::int64_t step) const -> Series
```

## What It Does

`unfold()` produces all the sliding windows of length `size` along axis `dim`,
advancing by `step` between windows, and stacks them into a new trailing axis.

For the row `[10, 20, 30, 40, 50, 60]` with `size = 3, step = 1`, the windows
are `[10,20,30]`, `[20,30,40]`, `[30,40,50]`, `[40,50,60]` — a `4 x 3` result.
Axis `dim` shrinks from `n` to `floor((n - size) / step) + 1` windows, and the
window contents become a brand-new last axis of length `size`.

This is the building block for anything that operates on local neighbourhoods:
a moving average is `unfold(...).mean(-1)`, an n-gram extractor is
`unfold(dim, n, 1)`, a strided patch view for convolution-style code is
`unfold` on each spatial axis.

If `size` is larger than the axis length there are zero windows and the result
has a `0` in that position rather than an error.

## Parameters

**dim** (`std::int64_t`)

The axis to slide along.

**size** (`std::int64_t`)

The window length. Becomes the size of the new trailing axis.

**step** (`std::int64_t`)

The stride between window starts. `step = 1` gives maximally overlapping
windows; `step = size` gives non-overlapping tiles.

## Returns

`Series`
    The input with axis `dim` replaced by the window count and a new last axis
    of length `size`. So a rank-`r` input becomes rank `r + 1`.

## Gradient

`unfold()` is differentiable. Because overlapping windows read the same source
element more than once, the backward is an overlap-add: each source position
accumulates the gradient of every window it appears in. With `step >= size`
(no overlap) every position appears exactly once and the gradient is passed
straight through.

## Examples

Example 1. Overlapping length-3 windows, `step = 1`.

```cpp
std::cout << pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}).unfold(0, 3, 1);
```

### Output

```text
pluss::Series(shape=[4,3], strides=[3,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 20, 30],
  [20, 30, 40],
  [30, 40, 50],
  [40, 50, 60]]
```

Example 2. `step = size` gives non-overlapping tiles.

```cpp
std::cout << pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}).unfold(0, 2, 2);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20],
  [30, 40],
  [50, 60]]
```

Example 3. On a matrix, `unfold` along axis `1` appends the window as a new
third axis, giving a `2 x 3 x 2` result (two rows, three windows each, window
length two).

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});

pluss::series::print(std::cout, m.unfold(1, 2, 1));
```

### Output

```text
Series(shape=[2,3,2], strides=[6,2,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data[0, :, :] =
⎡ 1.0000 2.0000 ⎤
⎢ 2.0000 3.0000 ⎥
⎣ 3.0000 4.0000 ⎦

data[1, :, :] =
⎡ 5.0000 6.0000 ⎤
⎢ 6.0000 7.0000 ⎥
⎣ 7.0000 8.0000 ⎦
```

Example 4. Overlapping windows accumulate gradient. Interior positions that
land in three windows collect `3`.

```cpp
auto x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0});
x.tracks_slope_(true);

x.unfold(0, 3, 1).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1, 2, 3, 3, 2, 1]
```

Example 5. With no overlap (`step = size`) each position appears once, so the
gradient is passed straight through.

```cpp
auto x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0});
x.tracks_slope_(true);

x.unfold(0, 2, 2).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1, 1, 1, 1, 1, 1]
```

## Gotchas And Failure Modes

- `unfold()` adds an axis. A rank-1 input becomes rank-2; plan the rest of your
  pipeline around the new trailing window axis.
- If `size` exceeds the axis length the result has `0` windows (an empty
  series), not an error.
- With `step < size` the windows overlap and the backward is an overlap-add,
  not an identity.

## Summary

`unfold(dim, size, step)` is the sliding-window view:

- axis `dim` becomes the window count; a new last axis holds each window
- `step = 1` overlaps maximally, `step = size` tiles without overlap
- differentiable, backward is overlap-add over the windows each position joins

## See Also

- [`Series::narrow_copy`](narrow_copy.md): a single contiguous slice rather than
  every window.
- [`Series::diff`](diff.md): first differences, another local-neighbourhood
  operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="trunc.html">
    <span class="label">Previous</span>
    <span class="title">trunc</span>
  </a>
  <a class="next-link" href="union1d.html">
    <span class="label">Next</span>
    <span class="title">union1d</span>
  </a>
</div>
