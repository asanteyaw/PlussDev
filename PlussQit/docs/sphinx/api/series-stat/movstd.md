<!-- pluss-hand-authored -->

# SeriesStatAccessor::movstd

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movstd() const
Series SeriesStatAccessor::movstd(std::int64_t window) const
Series SeriesStatAccessor::movstd(std::int64_t window, std::int64_t dim) const
Series SeriesStatProxy::movstd() const
Series SeriesStatProxy::movstd(std::int64_t window) const
Series SeriesStatProxy::movstd(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movstd()` is the square root of trailing population `movvar()`. It keeps the
input shape and reports local spread in the same units as the input. The
default is a three-value window along the last axis.

At startup, windows contain only available values. A one-value prefix has zero
spread; a two-value prefix uses divisor two.

## Parameters And Return Shape

`window` is the positive maximum trailing width. `dim` identifies the ordered
axis, defaults to the last axis, and supports negative indexing. The transform
returns one floating result per input position and therefore preserves shape.

## Examples

Example 1. The default computes the square root of each three-position moving
population variance.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << x.stat.movstd() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0.5, 0.81649658, 0.81649658]
```

Example 2. A width-two window reports the spread of each adjacent pair.

```cpp
std::cout << x.stat().movstd(2) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0.5, 0.5, 0.5]
```

The full three-value windows have variance `2/3`, hence standard deviation
`sqrt(2/3) = 0.81649658`.

Example 2. Windows run only along the selected axis.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.movstd(2, 1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0.5, 0.5],
  [0, 0.5, 0.5]]
```

## Gradient

For a nonconstant window, the local derivative is
`(x_i-window_mean)/(N*window_std)`, accumulated over overlapping windows. At
exactly zero spread the implementation uses a zero subgradient, avoiding a
division by zero where the mathematical derivative is not unique.

Example 3. Reverse mode differentiates every overlapping local spread.

```cpp
auto tracked = pluss::make_series({1.0, 2.0, 4.0});
tracked.tracks_slope_(true);
tracked.stat.movstd(2).sum().reverse();
std::cout << tracked.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5, 0, 0.5]
```

The middle input receives opposite contributions from the two adjacent
windows, which cancel exactly in this example.

## Gotchas And Failure Modes

- The estimator is population standard deviation, not sample standard deviation.
- Startup values are valid partial-window results, not padded estimates.
- `window<=0` fails explicitly.

## See Also

- [`movvar`](movvar.md): same local spread in squared units.
- [`std`](std.md): one standard deviation per reduced lane.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movmin.html"><span class="label">Previous</span><span class="title">movmin</span></a>
  <a class="next-link" href="movsum.html"><span class="label">Next</span><span class="title">movsum</span></a>
</div>
