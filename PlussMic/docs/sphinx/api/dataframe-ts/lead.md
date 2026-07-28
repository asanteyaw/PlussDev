<!-- pluss-hand-authored -->

# DataFrameTsAccessor::lead

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::lead(std::int64_t periods = 1) const -> DataFrame
```

## What It Does

Shifts every bound feature `periods` rows backward: `out[i] = x[i + periods]` where
that index is in bounds, `NULL` otherwise. A negative `periods` delegates to
[`lag()`](lag.md) with the sign flipped.

## Parameters

**periods** (`std::int64_t`, default `1`)

The number of rows to shift backward. Negative values behave as
[`lag(-periods)`](lag.md) instead.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature led by `periods` rows.

## Examples

Example 1. Lead a numeric feature by one row.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().lead(1);
std::cout << out;
```

### Output

```text
===========
     | load
-----------
0    | 20  
1    | 15  
2    | 30  
3    | NULL
-----------
[4 cs x 1 feat]
```

Each case now shows the next case's `load` value: case `0` shows case `1`'s `20`,
case `1` shows case `2`'s `15`, and so on. Case `3` has no successor, so it's
`NULL` — the `NULL` lands at the opposite end from [`lag()`](lag.md).

## Notes

- `lead(periods)` with a negative `periods` is exactly `lag(-periods)` — the two
  methods share one implementation, differing only in shift direction.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::lag`](lag.md): the mirror-image shift.
- [`DataFrame::shift`](../dataframe/shift.md): the equivalent top-level operation
  (a negative `shift` argument corresponds to a lead) outside the `ts()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="log_returns.html">
    <span class="label">Next</span>
    <span class="title">log_returns</span>
  </a>
</div>
