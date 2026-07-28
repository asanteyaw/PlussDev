<!-- pluss-hand-authored -->

# DataFrameTsAccessor::lag_block

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::lag_block(pluss::RangeView<std::int64_t> lags) const -> DataFrame
```

## What It Does

Computes several [`lag()`](lag.md) shifts of the accessor's (single) bound feature
at once, each as its own output column named `"lag" + <lag value>`. Requires at
least one lag, and requires exactly one feature to be bound (since every output
column is derived from that one feature's raw values).

## Parameters

**lags** (`pluss::RangeView<std::int64_t>`)

The lag distances to compute, in the order given. Must not be empty.

## Returns

`DataFrame`
    A new frame with one column per requested lag (named `"lag1"`, `"lag2"`, and so
    on), same case labels as the accessor's bound feature.

## Examples

Example 1. Compute two lags of the same feature at once.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().lag_block({1, 2});
std::cout << out;
```

### Output

```text
=================
     | lag1  lag2
-----------------
0    | NULL  NULL
1    | 10    NULL
2    | 20    10  
3    | 15    20  
-----------------
[4 cs x 2 feat]
```

`lag1` is the same as [`lag(1)`](lag.md): `[NULL, 10, 20, 15]`. `lag2` is
[`lag(2)`](lag.md): each row now shows the value from two rows earlier, so both
cases `0` and `1` (which have no row two positions back) are `NULL`, case `2` shows
case `0`'s `10`, and case `3` shows case `1`'s `20`.

## Notes

- Each requested lag becomes its own output column, letting you compare several lag
  depths of the same feature side by side in one call, rather than calling
  [`lag()`](lag.md) repeatedly and combining the results yourself.
- Output columns are always named `"lag" + <value>` — there's no way to customize
  the column names from this method directly.
- Requires exactly one bound feature; narrow with
  [`features()`](../dataframe/features.md) first if the source frame has more than
  one.

## See Also

- [`DataFrameTsAccessor::lag`](lag.md): the single-lag building block this method
  computes several of at once.
- [`DataFrameTsAccessor::rolling_block`](rolling_block.md): the equivalent "several
  at once" pattern for rolling window statistics instead of lags.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
