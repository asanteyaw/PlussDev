<!-- pluss-hand-authored -->

# SeriesStatAccessor::movmax

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movmax() const
Series SeriesStatAccessor::movmax(std::int64_t window) const
Series SeriesStatAccessor::movmax(std::int64_t window, std::int64_t dim) const
Series SeriesStatProxy::movmax() const
Series SeriesStatProxy::movmax(std::int64_t window) const
Series SeriesStatProxy::movmax(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movmax()` emits the largest value in each trailing window. Shape is preserved,
the current position is included, startup windows use the available prefix,
and the default is window three on the last axis.

This operation is often used as a rolling high-water mark or as one-dimensional
morphological dilation. It does not report the index of the winning value.

## Parameters And Return Shape

`window` is the maximum trailing width and must be positive. `dim` identifies
the ordered axis; it defaults to the last axis and accepts negative indexing.
The result preserves shape and dtype because it selects existing values rather
than averaging them.

## Examples

Example 1. On rising data, every current value is already the largest member of
its trailing window.

```cpp
auto rising = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << rising.stat.movmax() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 4]
```

Example 2. A high value persists until it leaves the trailing window.

```cpp
auto mixed = pluss::make_series({3.0, 1.0, 4.0, 2.0, 0.0});
std::cout << mixed.stat().movmax(3) << "\n";
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[3, 3, 4, 4, 4]
```

The final output remains `4` because the trailing windows at positions three
and four still contain the value at position two.

## Gradient And Ties

Reverse mode sends an output's sensitivity to its maximizing input. If several
values tie for the maximum in the same window, sensitivity is split evenly
among those ties. An input can accumulate contributions from several outputs.

That tie split is a deliberate subgradient convention: the mathematical
maximum is nondifferentiable where winners tie.

Example 3. The equal-share rule is visible when two maxima tie.

```cpp
auto x = pluss::make_series({2.0, 2.0, 1.0});
x.tracks_slope_(true);
x.stat.movmax(2).sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5, 1.5, 0]
```

The first output belongs entirely to the first `2`; the second output splits
between both `2`s; the third output also splits between them. Those three
contributions total `1.5` for each tied value.

## Gotchas And Failure Modes

- `movmax()` is causal/trailing; future values never affect earlier outputs.
- Tied maxima share gradients rather than choosing the first occurrence.
- `window` must be positive.

## See Also

- [`movmin`](movmin.md): trailing low-water mark.
- [`movmean`](movmean.md): smooth every value rather than select a winner.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html"><span class="label">Previous</span><span class="title">mean</span></a>
  <a class="next-link" href="movmean.html"><span class="label">Next</span><span class="title">movmean</span></a>
</div>
