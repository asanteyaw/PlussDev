<!-- pluss-hand-authored -->

# TimeSeriesAccessor::rolling_block

[Back to TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference)

## Signature

```cpp
DataFrame TimeSeriesAccessor::rolling_block(pluss::RangeView<std::int64_t> windows, const TsRollingBlockOptions& options = {}) const
```

## What It Does

Computes several rolling window statistics, at several window sizes, of the
bound shelf all at once — each `{statistic, window}` combination becomes its
own output column named `"roll_" + <statistic> + "_w" + <window>`. Requires at
least one window.

```{important} does not use the same engine as rolling()
Unlike [`rolling()`](rolling.md), this method's statistics do **not** skip
missing values within a window — like [`zscore_rolling()`](zscore_rolling.md),
they accumulate every position in the window with plain floating-point
arithmetic, so one missing entry makes every column's value come back `NULL`
for every position whose window still contains it. See
[`zscore_rolling()`](zscore_rolling.md)'s important callout for a fully worked
example of this exact effect.
```

## Parameters

**windows** (`pluss::RangeView<std::int64_t>`)

The window sizes to compute, in the order given. Must not be empty, or it
throws.

**options** (`const TsRollingBlockOptions&`)

Which statistics to include, and the shared `min_periods`:

- **`include_mean`** (`bool`, default `true`)
- **`include_std`** (`bool`, default `true`)
- **`include_min`** (`bool`, default `false`)
- **`include_max`** (`bool`, default `false`)
- **`include_sum`** (`bool`, default `false`)
- **`include_var`** (`bool`, default `false`)
- **`min_periods`** (`std::int64_t`, default `1`): the minimum number of
  positions that must structurally exist in a window near the start of the
  shelf — see [`zscore_rolling()`](zscore_rolling.md)'s parameter note for why
  this isn't the same as "non-missing observations."

## Returns

`DataFrame`
    A new frame with one column per requested `{statistic, window}` pair, same
    case labels as the bound shelf. Columns are ordered by window (in the
    order given), then by statistic in the fixed order mean, std, min, max,
    sum, var.

## Examples

Example 1. The default statistics (`mean`, `std`) across two window sizes.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = values.ts().rolling_block({2, 3});
std::cout << out;
```

### Output

```text
===========================================================
     | roll_mean_w2  roll_std_w2  roll_mean_w3  roll_std_w3
-----------------------------------------------------------
0    | 10            NULL         10            NULL       
1    | 15            7.071067812  15            7.071067812
2    | 25            7.071067812  20            10         
3    | 35            7.071067812  30            10         
4    | 45            7.071067812  40            10         
-----------------------------------------------------------
[5 cs x 4 feat]
```

`roll_std_w2`/`roll_std_w3` use a **population** convention (divide by count,
not count `- 1`), same as [`zscore_rolling()`](zscore_rolling.md) — there's no
`ddof` to change that here.

Example 2. Toggle which statistics are included — `min`, `max`, `sum`, and
`var` instead of the defaults.

```cpp
auto out2 = values.ts().rolling_block(
    {3},
    TsRollingBlockOptions{
        .include_mean = false, .include_std = false,
        .include_min = true, .include_max = true,
        .include_sum = true, .include_var = true});
std::cout << out2;
```

### Output

```text
=========================================================
     | roll_min_w3  roll_max_w3  roll_sum_w3  roll_var_w3
---------------------------------------------------------
0    | 10           10           10           NULL       
1    | 10           20           30           50         
2    | 10           30           60           100        
3    | 20           40           90           100        
4    | 30           50           120          100        
---------------------------------------------------------
[5 cs x 4 feat]
```

Example 3. An empty `windows` throws.

```cpp
try {
  auto bad = values.ts().rolling_block({});
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
TimeSeriesAccessor::rolling_block: expected at least one window
```

## Notes

- See the important callout above: this method's rolling statistics poison an
  entire window on a single missing value, rather than skipping it the way
  [`rolling()`](rolling.md) does.
- `std`/`var` here always use a population convention (divide by count, not
  count `- 1`).
- Column names are fixed (`"roll_" + statistic + "_w" + window`) and column
  order follows `windows`, then the fixed statistic order mean/std/min/max/
  sum/var — there's no way to rename or reorder them from this method
  directly.

## See Also

- [`TimeSeriesAccessor::rolling`](rolling.md): a single window/statistic at a
  time, using the skip-missing-values engine instead.
- [`TimeSeriesAccessor::lag_block`](lag_block.md): the equivalent "several at
  once" pattern for lags instead of rolling statistics.
- [`TimeSeriesAccessor::zscore_rolling`](zscore_rolling.md): shares this
  method's underlying rolling-mean/rolling-std engine and its
  missing-value-poisons-the-window behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling.html">
    <span class="label">Previous</span>
    <span class="title">rolling</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
