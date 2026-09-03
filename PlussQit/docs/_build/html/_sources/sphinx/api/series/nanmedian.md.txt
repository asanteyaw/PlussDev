<!-- pluss-hand-authored -->

# Series::nanmedian

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nanmedian() const -> Series
Series::nanmedian(std::int64_t dim) const -> Series
```

## What It Does

`nanmedian()` is median after NaN values have been removed. Every reduction
lane is filtered independently, ordered, then reduced: an odd valid count
selects one centre and an even valid count averages two centres. It skips NaN
only; `inf` and `-inf` remain valid sortable values.

If filtering leaves no valid value in a population, the result is NaN. This is
important: an all-missing lane is not silently converted to zero.

## Overload Map

| Call | Valid population | Result |
| --- | --- | --- |
| `x.nanmedian()` | all non-NaN values | scalar Series |
| `x.nanmedian(dim)` | non-NaN values per lane of `dim` | shape without `dim` |

## Parameters

**dim** (`std::int64_t`) identifies the reduced axis; negative values count
from the end. No `keepdim` toggle is provided.

## Returns

A new `float64` Series. The input remains unchanged.

## Gradient

NaN filtering, ordering, and middle-rank selection are discontinuous choices.
Treat this as a descriptive robust statistic rather than a smooth objective.

## Examples

Example 1. NaN does not enter the ordered population.

```cpp
auto missing = pluss::make_series({
    1.0, std::numeric_limits<double>::quiet_NaN(), 5.0, 9.0});
pluss::series::print(std::cout, missing.nanmedian());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.0000
```

The valid population `[1, 5, 9]` has `5` as its centre.

Example 2. Each column can retain a different number of observations.

```cpp
auto table = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 9.0},
    {3.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
});
std::cout << table.nanmedian(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 5, 9]
```

The first lane uses `[1, 3]`, so it returns `(1 + 3) / 2 = 2`.

Example 3. The difference from `median()` is exactly the NaN policy.

```cpp
std::cout << missing.median() << "\n";
std::cout << missing.nanmedian();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 4. No valid values means no definable centre.

```cpp
auto absent = pluss::make_series({
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN()});
std::cout << absent.nanmedian();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 5. Axis validation remains strict.

```cpp
try { std::cout << table.nanmedian(2); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
quantile: dim out of range
```

## Gotchas And Failure Modes

- Only NaN is ignored; infinities can be medians.
- Every all-NaN lane returns NaN.
- The output is `float64`, including for integral input.

## Summary

Use `nanmedian()` when NaN means missing observation. It filters each lane
independently, follows the normal odd/even median rule, and returns NaN where
no valid observations survive.

## See Also

- [`Series::median`](median.md)
- [`Series::nanquantile`](nanquantile.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanmean.html">
    <span class="label">Previous</span>
    <span class="title">nanmean</span>
  </a>
  <a class="next-link" href="nanquantile.html">
    <span class="label">Next</span>
    <span class="title">nanquantile</span>
  </a>
</div>
