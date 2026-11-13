<!-- pluss-hand-authored -->

# Series::quantile

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::quantile() const
Series Series::quantile(double q) const
```

## What It Does

`quantile()` reports a fractional position in one globally ordered population.
The default is `q = 0.5`, the same global result as [`median()`](median.md).
`q = 0` is the minimum and `q = 1` is the maximum.

For `n` ordered values, it calculates `p = q(n - 1)`. If `p` lies between
integer positions `i` and `i + 1`, it linearly interpolates:

```text
sorted[i] + (p - i) * (sorted[i + 1] - sorted[i])
```

The returned quantile can therefore lie between observed values.

## Parameters

**q** (`double`) is the requested fraction and must be in `[0.0, 1.0]`.
The root API is global only: there are no `dim`, `keepdim`, or interpolation
policy toggles in this overload family.

## Returns

A scalar `float64` Series. `quantile()` does not modify the source.

## Gradient

Sorting and rank selection change discretely when values cross or tie. Quantile
is a distribution statistic, not a smooth differentiable reduction.

## Examples

Example 1. The default is the 0.5 quantile.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, values.quantile());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.5000
```

Example 2. A first quartile uses the same formula rather than selecting an
arbitrary neighbour.

```cpp
std::cout << values.quantile(0.25);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.75
```

Here `p = 0.25(4 - 1) = 0.75`, so the result is `1 + 0.75(2 - 1) = 1.75`.

Example 3. Endpoint fractions are exact extrema.

```cpp
std::cout << values.quantile(0.0) << "\n";
std::cout << values.quantile(1.0);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

Example 4. Plain quantile propagates NaN; the `nan` form filters first.

```cpp
auto missing = pluss::make_series({
    1.0, std::numeric_limits<double>::quiet_NaN(), 5.0, 9.0});
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

Example 5. Fractions outside the closed unit interval throw.

```cpp
try { std::cout << values.quantile(1.1); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
quantile: q must be between 0 and 1
```

## Gotchas And Failure Modes

- This is global: it has no axis parameter.
- NaN propagates. Use [`nanquantile()`](nanquantile.md) for missing values.
- The result may not have appeared in the source because it is interpolated.
- Out-of-range `q` is rejected rather than clipped.

## Summary

Use `quantile(q)` for a global fractional-rank statistic. It orders all values,
interpolates at `q(n - 1)`, returns a scalar `float64` Series, and keeps NaN
semantics explicit.

## See Also

- [`Series::nanquantile`](nanquantile.md)
- [`Series::median`](median.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put_.html">
    <span class="label">Previous</span>
    <span class="title">put_</span>
  </a>
  <a class="next-link" href="random_.html">
    <span class="label">Next</span>
    <span class="title">random_</span>
  </a>
</div>
