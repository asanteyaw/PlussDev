<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5c764254e26e -->

# DataFrameTsAccessor::zscore_rolling

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12) const
```

## What It Does

Computes a trailing rolling z-score for every bound feature:
`out[i] = (x[i] - rolling_mean[i]) / rolling_std[i]`, where the rolling mean and
standard deviation are the same trailing-window statistics
[`rolling()`](rolling.md) computes (`ddof = 1`, partial windows honored). A cell is
`NULL` if `x[i]`, the rolling mean, or the rolling standard deviation is undefined,
or if the rolling standard deviation's magnitude doesn't exceed `epsilon`.

## Parameters

**window** (`std::int64_t`)

The trailing window size, in rows, used for both the rolling mean and rolling
standard deviation.

**min_periods** (`std::int64_t`, default `1`)

Minimum non-null observations required in the window for both the mean and
standard deviation to be defined.

**epsilon** (`double`, default `1e-12`)

The minimum rolling standard deviation magnitude treated as non-zero; a smaller
magnitude produces `NULL` rather than dividing by (near) zero.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its rolling z-score.

## Examples

Example 1. A window-2 rolling z-score.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().zscore_rolling(2);
std::cout << out;
```

### Output

```text
====================
     | load         
--------------------
0    | NULL         
1    | 0.7071067812 
2    | -0.7071067812
3    | 0.7071067812 
--------------------
[4 cs x 1 feat]
```

Row `0`'s window has only 1 observation, so its rolling standard deviation is
undefined (the default `ddof = 1` requires more than 1 observation) and the
z-score is `NULL`. Row `1`'s window is `(10, 20)`: mean `15`, std ≈ `7.0711`, so
`z = (20 - 15) / 7.0711 ≈ 0.7071`. Row `2`'s window is `(20, 15)`: mean `17.5`,
std ≈ `3.5355`, so `z = (15 - 17.5) / 3.5355 ≈ -0.7071`. Row `3`'s window is
`(15, 30)`: mean `22.5`, std ≈ `10.6066`, so `z = (30 - 22.5) / 10.6066 ≈ 0.7071`.

## Notes

- `zscore_rolling()` is built directly from the same [`rolling()`](rolling.md)
  mean/std computation — every partial-window and `min_periods` behavior
  documented there applies identically here for both the numerator and
  denominator.
- `epsilon` guards only against a (near-)zero rolling standard deviation, not
  against `x[i]` or the rolling mean being undefined — either of those still
  produces `NULL` regardless of `epsilon`.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the underlying rolling mean/std this
  method combines into a z-score.
- [`DataFrame::rank`](../dataframe/rank.md): a different way of normalizing values,
  by relative order rather than a rolling distributional statistic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="window.html">
    <span class="label">Previous</span>
    <span class="title">window</span>
  </a>
</div>
