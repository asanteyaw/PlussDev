<!-- pluss-hand-authored -->

# SeriesStatAccessor::movmin

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movmin() const
Series SeriesStatAccessor::movmin(std::int64_t window) const
Series SeriesStatAccessor::movmin(std::int64_t window, std::int64_t dim) const
Series SeriesStatProxy::movmin() const
Series SeriesStatProxy::movmin(std::int64_t window) const
Series SeriesStatProxy::movmin(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movmin()` emits the smallest value in each trailing window. It is the rolling
low-water counterpart to `movmax()`: same shape, same partial-prefix startup,
same default window three on the last axis.

## Parameters And Return Shape

`window` is the maximum number of trailing positions and must be positive.
`dim` selects the ordered axis, defaults to the last axis, and accepts negative
indexing. Since each output selects an input value, shape and dtype are
preserved.

## Examples

Example 1. A low value remains active until it leaves the window.

```cpp
auto rising = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << rising.stat.movmin() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 2]
```

Example 2. New lows replace the old low-water mark immediately.

```cpp
auto mixed = pluss::make_series({3.0, 1.0, 4.0, 2.0, 0.0});
std::cout << mixed.stat().movmin(3) << "\n";
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[3, 1, 1, 1, 0]
```

In the rising series, value `1` remains in the first three windows. It leaves
the window at the final position, so the minimum advances to `2`.

## Axis Example

```cpp
auto x = pluss::make_series({{3.0, 2.0, 1.0}, {6.0, 5.0, 4.0}});
std::cout << x.stat.movmin(2, 1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 2, 1],
  [6, 5, 4]]
```

Each row is processed independently along `dim=1`.

## Gradient And Ties

Each output sends sensitivity to its minimizing input. Equal minima divide that
sensitivity evenly. Because windows overlap, one low value can collect gradient
from several subsequent positions.

```cpp
auto x = pluss::make_series({1.0, 1.0, 2.0});
x.tracks_slope_(true);
x.stat.movmin(2).sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5, 1.5, 0]
```

The first minimum contributes to the first `1`; the tied second window divides
its contribution; and the final window belongs to the second `1`.

## Gotchas And Failure Modes

- The operation returns values, not argmin positions.
- Tied minima use an equal-share subgradient.
- Omitted `dim` means the last axis; `window` must be positive.

## See Also

- [`movmax`](movmax.md): trailing maximum.
- [`movvar`](movvar.md): trailing spread rather than an extreme.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movmean.html"><span class="label">Previous</span><span class="title">movmean</span></a>
  <a class="next-link" href="movstd.html"><span class="label">Next</span><span class="title">movstd</span></a>
</div>
