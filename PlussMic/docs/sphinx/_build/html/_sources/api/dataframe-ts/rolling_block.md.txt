<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 54dc8cc06f06 -->

# DataFrameTsAccessor::rolling_block

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::rolling_block(pluss::RangeView<std::int64_t> windows, const TsRollingBlockOptions& options = {}) const -> DataFrame
```

## What It Does

Computes several rolling-window statistics, over several window sizes, of the
accessor's (single) bound feature in one call — each combination becomes its own
output column, named `"roll_{stat}_w{window}"` (for example `"roll_mean_w2"`).
Requires exactly one feature to be bound.

## Parameters

**windows** (`pluss::RangeView<std::int64_t>`)

The window sizes to compute, in the order given. Must not be empty.

**options** (`const TsRollingBlockOptions&`)

Which statistics to compute for every window size, and the shared `min_periods`:

- **`include_mean`** (`bool`, default `true`)
- **`include_std`** (`bool`, default `true`)
- **`include_min`** (`bool`, default `false`)
- **`include_max`** (`bool`, default `false`)
- **`include_sum`** (`bool`, default `false`)
- **`include_var`** (`bool`, default `false`)
- **`min_periods`** (`std::int64_t`, default `1`)

## Returns

`DataFrame`
    A new frame with one column per requested `(window, included statistic)`
    combination, same case labels as the accessor's bound feature.

## Examples

Example 1. The default statistics (mean and standard deviation) for one window
size.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().rolling_block({2});
std::cout << out;
```

### Output

```text
================================
     | roll_mean_w2  roll_std_w2
--------------------------------
0    | 10            NULL       
1    | 15            7.071067812
2    | 17.5          3.535533906
3    | 22.5          10.60660172
--------------------------------
[4 cs x 2 feat]
```

`roll_mean_w2` matches [`rolling(2)`](rolling.md)'s default mean output exactly.
`roll_std_w2` matches [`rolling(2, WindowStatistic::Std)`](rolling.md): row `0`'s
single-observation window has an undefined standard deviation with the default
`ddof = 1`, so it's `NULL`, and the remaining rows are the sample standard
deviation of each trailing pair.

## Notes

- Each `(window, statistic)` combination becomes its own column — with several
  window sizes and several enabled statistics, the output can grow quickly
  (`windows.size() * (number of enabled statistics)` columns).
- Column names are fixed to the `"roll_{stat}_w{window}"` pattern and aren't
  customizable from this method directly.
- `min_periods` (default `1`) is shared across every window size and statistic in
  the call — there's no way to vary it per window size in one `rolling_block()`
  call.
- Requires exactly one bound feature; narrow with
  [`features()`](../dataframe/features.md) first if the source frame has more than
  one.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the single-window, single-statistic
  building block this method computes several combinations of at once.
- [`DataFrameTsAccessor::lag_block`](lag_block.md): the equivalent "several at once"
  pattern for lags instead of rolling statistics.

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
