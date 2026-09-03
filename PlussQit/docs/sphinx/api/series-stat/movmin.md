<!-- pluss-hand-authored -->

# SeriesStatAccessor::movmin

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movmin() const
Series SeriesStatAccessor::movmin(std::int64_t window) const
Series SeriesStatAccessor::movmin(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movmin()` emits the smallest value in each trailing window. It is the rolling
low-water counterpart to `movmax()`: same shape, same partial-prefix startup,
same default window three on the last axis.

## Examples

```cpp
auto rising = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto mixed = pluss::make_series({3.0, 1.0, 4.0, 2.0, 0.0});
std::cout << rising.stat.movmin() << "\n";
std::cout << mixed.stat().movmin(3) << "\n";
```

### Output

```text
data=[1, 1, 1, 2]
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
data=[[3, 2, 1],
  [6, 5, 4]]
```

Each row is processed independently along `dim=1`.

## Gradient And Ties

Each output sends sensitivity to its minimizing input. Equal minima divide that
sensitivity evenly. Because windows overlap, one low value can collect gradient
from several subsequent positions.

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
