<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c8582289a30 -->

# DataFrameTsAccessor::lag

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::lag(std::int64_t periods = 1) const -> DataFrame
```

## What It Does

Shifts every bound feature `periods` rows forward: `out[i] = x[i - periods]` where
that index is in bounds, `NULL` otherwise. A negative `periods` delegates to
[`lead()`](lead.md) with the sign flipped.

## Parameters

**periods** (`std::int64_t`, default `1`)

The number of rows to shift forward. Negative values behave as
[`lead(-periods)`](lead.md) instead.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature lagged by `periods` rows.

## Examples

Example 1. Lag a numeric feature by one row.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().lag(1);
std::cout << out;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | 20  
3    | 15  
-----------
[4 cs x 1 feat]
```

Each case now shows the previous case's `load` value: case `1` shows case `0`'s
`10`, case `2` shows case `1`'s `20`, and so on. Case `0` has no predecessor, so
it's `NULL`. This is the same operation the top-level
[`DataFrame::shift(periods)`](../dataframe/shift.md) performs when scoped to these
features.

## Notes

- `lag(periods)` with a negative `periods` is exactly `lead(-periods)` — the two
  methods share one implementation, differing only in shift direction.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::lead`](lead.md): the mirror-image shift.
- [`DataFrameTsAccessor::lag_block`](lag_block.md): computing several lags of the
  same feature at once, each as its own output column.
- [`DataFrame::shift`](../dataframe/shift.md): the equivalent top-level operation
  outside the `ts()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="insert_gaps.html">
    <span class="label">Previous</span>
    <span class="title">insert_gaps</span>
  </a>
  <a class="next-link" href="lag_block.html">
    <span class="label">Next</span>
    <span class="title">lag_block</span>
  </a>
</div>
