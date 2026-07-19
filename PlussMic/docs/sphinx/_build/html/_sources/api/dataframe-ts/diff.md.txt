<!-- pluss-hand-authored -->

# DataFrameTsAccessor::diff

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::diff(std::int64_t periods = 1) const -> DataFrame
```

## What It Does

Computes the absolute period-over-period change of every bound feature:
`out[i] = x[i] - base[i]`, where `base` is `x` shifted by `periods` (a lag when
`periods > 0`, a lead when `periods < 0`). A cell is `NULL` wherever `x[i]` or
`base[i]` is missing. `periods = 0` produces all zeros rather than throwing.

## Parameters

**periods** (`std::int64_t`, default `1`)

The shift distance used to build the comparison base. Positive shifts compare each
row against an earlier one (a lag); negative shifts compare against a later one (a
lead).

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its period-over-period absolute difference.

## Examples

Example 1. Compute the one-period difference of a numeric feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().diff(1);
std::cout << out;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | -5  
3    | 15  
-----------
[4 cs x 1 feat]
```

Case `0` has no prior row, so it's `NULL`. Case `1` is `20 - 10 = 10`; case `2` is
`15 - 20 = -5`; case `3` is `30 - 15 = 15`.

## Notes

- Unlike [`shift()`](../dataframe/shift.md)-based approaches you might build by
  hand, `diff()` handles `periods = 0` explicitly by returning all zeros (an
  identity comparison) rather than a frame of `NULL`.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::returns`](returns.md),
  [`DataFrameTsAccessor::log_returns`](log_returns.md): the fractional and
  logarithmic counterparts to this absolute difference.
- [`DataFrameTsAccessor::lag`](lag.md), [`DataFrameTsAccessor::lead`](lead.md): the
  underlying shift this method's comparison base is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="calendar.html">
    <span class="label">Previous</span>
    <span class="title">calendar</span>
  </a>
  <a class="next-link" href="div.html">
    <span class="label">Next</span>
    <span class="title">div</span>
  </a>
</div>
