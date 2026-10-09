<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a73805824ada -->

# StreamTimeSeriesAccessor::rolling_block

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame StreamTimeSeriesAccessor::rolling_block(
    pluss::msharp::RangeView::Int windows,
    const pluss::table::TsRollingBlockOptions& options = {}) const
```

## What It Does

Computes several rolling window statistics, at several window sizes, of the
bound stream all at once — each `{statistic, window}` combination becomes its
own output column named `"roll_" + <statistic> + "_w" + <window>`. Requires
at least one window.

```{important} Returns a DataFrame, not a Stream
Same escape from the kindle model as [`lag_block()`](lag_block.md) — see that
page's own important callout.
```

```{important} does not use the same engine as rolling()
Unlike [`rolling()`](rolling.md), this method's statistics do **not** skip
missing values within a window — they accumulate every position in the
window with plain floating-point arithmetic, so one missing entry makes
every column's value come back `NULL` for every position whose window still
contains it. See [`zscore_rolling()`](zscore_rolling.md)'s important callout
for a fully worked example of this exact effect.
```

## Parameters

**windows** (`pluss::msharp::RangeView::Int`)
    The window sizes to compute, in the order given. Must not be empty, or
    it throws.

**options** (`const pluss::table::TsRollingBlockOptions&`)
    `include_mean`/`include_std` (default `true`), `include_min`/
    `include_max`/`include_sum`/`include_var` (default `false`), and
    `min_periods` (default `1`).

## Returns

`pluss::table::DataFrame`
    A new frame with one column per requested `{statistic, window}` pair,
    ordered by window then by the fixed statistic order
    mean/std/min/max/sum/var.

## Examples

Example 1. The default statistics (`mean`, `std`) across two window sizes.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = stream.ts().rolling_block({2, 3});
std::cout << out;
```

### Output

```text
===========================================================
     | roll_mean_w2  roll_std_w2  roll_mean_w3  roll_std_w3
-----------------------------------------------------------
0    | 10            0            10            0          
1    | 15            5            15            5          
2    | 25            5            20            8.164965809
3    | 35            5            30            8.164965809
4    | 45            5            40            8.164965809
-----------------------------------------------------------
[5 cs x 4 feat]
```

`roll_std_w2`/`roll_std_w3` use a **population** convention (divide by
count, not count `- 1`), same as [`zscore_rolling()`](zscore_rolling.md).

## Notes

- See the important callouts above.
- `std`/`var` here always use a population convention.
- Column names are fixed and column order follows `windows`, then the fixed
  statistic order mean/std/min/max/sum/var.

## See Also

- [`StreamTimeSeriesAccessor::rolling`](rolling.md): a single window/statistic
  at a time, using the skip-missing-values engine instead.
- [`StreamTimeSeriesAccessor::lag_block`](lag_block.md): the equivalent
  "several at once" pattern for lags.
- [`StreamTimeSeriesAccessor::zscore_rolling`](zscore_rolling.md): shares
  this method's underlying rolling-mean/rolling-std engine.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::rolling_block`](../slate-ts/rolling_block.md): the
  underlying single-slate engine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling.html">
    <span class="label">Previous</span>
    <span class="title">rolling</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
