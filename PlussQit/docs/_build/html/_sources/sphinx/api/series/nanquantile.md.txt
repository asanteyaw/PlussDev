<!-- pluss-hand-authored -->

# Series::nanquantile

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nanquantile() const -> Series
Series::nanquantile(double q) const -> Series
```

## What It Does

`nanquantile()` is the global quantile calculation after NaN values have been
removed. It uses the same fractional position and interpolation as
[`quantile()`](quantile.md), but calculates the ordered population and its size
from non-NaN values only. Its no-argument form means `q = 0.5`, so it is the
same statistic as [`nanmedian()`](nanmedian.md).

If the valid ordered values are `v[0] ... v[n - 1]`, it evaluates
`p = q(n - 1)` and interpolates between adjacent values when `p` is not an
integer. Infinities remain valid values; only NaN is omitted.

## Parameters

**q** (`double`) is a global fractional rank in the closed interval `[0, 1]`.
There is no axis or `keepdim` overload in this root family.

## Returns

A scalar `float64` Series, or scalar NaN when no valid observation remains.

## Gradient

Filtering NaNs, ordering, and choosing neighbouring ranks make this a
non-smooth descriptive reduction. Do not rely on it as a smooth objective.

## Examples

Example 1. The median quantile ignores an unavailable observation.

```cpp
auto missing = pluss::make_series({
    1.0, std::numeric_limits<double>::quiet_NaN(), 5.0, 9.0});
pluss::series::print(std::cout, missing.nanquantile(0.5));
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.0000
```

The working population is `[1, 5, 9]`, so the central rank is `5`.

Example 2. The zero-argument overload is precisely `q = 0.5`.

```cpp
std::cout << missing.nanquantile();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 3. The choice between the two families is a missing-data decision.

```cpp
std::cout << missing.quantile(0.5) << "\n";
std::cout << missing.nanquantile(0.5);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 4. The fraction contract is the same as ordinary quantile.

```cpp
try { std::cout << missing.nanquantile(-0.25); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
quantile: q must be between 0 and 1
```

## Gotchas And Failure Modes

- Only NaN is omitted; positive and negative infinity remain ordered values.
- An all-NaN input has no valid population and yields NaN.
- The method is global, not per-axis.
- Invalid `q` throws instead of being clipped.

## Summary

Use `nanquantile(q)` for a global fractional-rank statistic when NaN means
missing. It uses the normal quantile interpolation on valid values only and
makes the all-missing case visible as NaN.

## See Also

- [`Series::quantile`](quantile.md)
- [`Series::nanmedian`](nanmedian.md)
