<!-- pluss-hand-authored -->

# Series::nanmean

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nanmean() const -> Series
Series::nanmean(std::int64_t dim) const -> Series
Series::nanmean(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`nanmean()` averages the values along an axis, ignoring `NaN` entries entirely.

It is [`mean()`](mean.md) with missing data skipped: the total is over the
finite values only, and — crucially — so is the divisor. The average of
`[1, NaN, 3, NaN, 5]` is `(1 + 3 + 5) / 3 = 3`, not `/ 5`.

A reduced axis with no finite values at all gives `NaN` (a `0 / 0`), which is
the one place `nanmean()` still produces a `NaN`.

The no-argument form averages the whole series to a scalar. The `dim` forms
reduce one axis, with `keepdim` controlling whether it is kept as a length-1
dimension.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to reduce. Negative values count from the back. Omitted: reduce
everything to a scalar.

**keepdim** (`bool`, default `false`)

Keep the reduced axis as size `1` instead of removing it.

## Returns

`Series`
    The `NaN`-skipping average. A scalar for the no-argument form; otherwise the
    input shape with `dim` removed (or kept as `1`). An all-`NaN` reduced axis
    gives `NaN`.

## Gradient

`nanmean()` is differentiable. Each finite position in a reduced axis receives
`1 / k` of the upstream sensitivity, where `k` is the number of finite values
in that axis; `NaN` positions receive `0`:

```
d/dx_i nanmean(x) = 0 if x_i is NaN, else 1 / (count of finite values)
```

## Examples

Example 1. The divisor is the count of finite values, not the axis length.

```cpp
double nan = std::nan("");

pluss::series::print(
    std::cout,
    pluss::make_series({1.0, nan, 3.0, nan, 5.0}).nanmean());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.0000
```

`(1 + 3 + 5) / 3 = 3`.

Example 2. An axis with no finite values gives `NaN`.

```cpp
double nan = std::nan("");

pluss::series::print(std::cout, pluss::make_series({nan, nan}).nanmean());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

This is the difference from [`nansum()`](nansum.md), which returns `0` in the
same case.

Example 3. Reducing one axis of a matrix, with a different finite count per
lane.

```cpp
double nan = std::nan("");
auto m = pluss::make_series({{1.0, nan}, {3.0, 4.0}});

pluss::series::print(std::cout, m.nanmean(1));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 3.5000]
```

Row `0` has one finite value, so its mean is `1`. Row `1` has two, so its mean
is `3.5`.

Example 4. The backward gives each finite position `1 / k` and each `NaN`
position `0`.

```cpp
double nan = std::nan("");
auto x = pluss::make_series({1.0, nan, 3.0, 5.0});
x.tracks_slope_(true);

x.nanmean().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.33333333, 0, 0.33333333, 0.33333333]
```

There are three finite values, so each gets `1/3`.

Example 5. The per-lane count carries through the backward too.

```cpp
double nan = std::nan("");
auto x = pluss::make_series({{1.0, nan}, {3.0, 4.0}});
x.tracks_slope_(true);

x.nanmean(1).sum().reverse();

std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0.5, 0.5]]
```

Row `0` has one finite value (gradient `1`), row `1` has two (gradient `0.5`
each), and the `NaN` gets `0`.

## Gotchas And Failure Modes

- An all-`NaN` axis gives `NaN`, unlike [`nansum()`](nansum.md) which gives
  `0`.
- `+inf` and `-inf` are not skipped; only `NaN` is.
- Floating-point input.

## Summary

`nanmean(x)` averages an axis over its finite values only:

- divisor is the finite count, not the axis length
- all-`NaN` axis reduces to `NaN`
- differentiable, backward is `1 / count` on finite positions and `0` on `NaN`
  positions

## See Also

- [`Series::mean`](mean.md): the plain average, which returns `NaN` on any
  `NaN` input.
- [`Series::nansum`](nansum.md): the `NaN`-skipping total (`0` for an all-`NaN`
  axis).
- [`Series::nanmedian`](nanmedian.md), [`Series::nanvar`](nanvar.md): other
  `NaN`-skipping statistics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nan_to_num.html">
    <span class="label">Previous</span>
    <span class="title">nan_to_num</span>
  </a>
  <a class="next-link" href="nanmedian.html">
    <span class="label">Next</span>
    <span class="title">nanmedian</span>
  </a>
</div>
