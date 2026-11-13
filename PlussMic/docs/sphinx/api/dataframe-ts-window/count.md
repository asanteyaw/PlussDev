<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 13c902750c9f -->

# DataFrameTsWindowAccessor::count

[Back to DataFrameTsWindowAccessor API Reference](../../dataframe_ts_window_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsWindowAccessor::count(std::string_view value_feature = {}) const
```

## What It Does

Computes, **once per group**, the number of observations of `value_feature` over
the trailing duration-length window ending at that group's own most recent
time-axis value (or an explicit `TsOptions.anchor`, if one was set) — the same
per-group trailing window [`sum()`](sum.md) uses, with `Count` in place of `Sum`.
Unlike the other `DataFrameTsWindowAccessor` methods, `value_feature` doesn't need
to be numeric for `count()`, since it only counts observations rather than
combining their values.

## Parameters

**value_feature** (`std::string_view`, default empty)

The feature to count observations of. May be omitted only if exactly one feature
is currently bound on the accessor.

## Returns

`DataFrame`
    One row per distinct group, with a group-key column (or columns) and a
    `"count_" + value_feature` column.

## Examples

Example 1. Count each sensor's readings over the trailing 2 days ending at that
sensor's own last reading — the same dataset as [`sum()`](sum.md).

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sensor_id", Slate(pluss::make_note({"A", "A", "A", "B", "B", "B"}))},
     {"value",     Slate(pluss::make_series({10.0, 20.0, 15.0, 5.0, 8.0, 12.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
        "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"})));

auto groups = readings.features("sensor_id");
auto out = readings.features({"value"}).ts({.group = groups}).window("2d").count();
std::cout << out;
```

### Output

```text
=============================
     | sensor_id  count_value
-----------------------------
0    | A          2          
1    | B          2          
-----------------------------
[2 cs x 2 feat]
```

Sensor `A`'s trailing window holds 2 observations (`20`, `15`); the earlier
reading (`10`, from `2024-01-01`) falls outside the `(2024-01-02, 2024-01-04]`
window, so it isn't counted. Sensor `B`'s window similarly holds 2 observations.

Example 2. Count a `Note`-kind (text) feature — unlike `sum`/`mean`/`min`/`max`,
`count()` doesn't require `value_feature` to be numeric.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sensor_id", Slate(pluss::make_note({"A", "A", "A", "B", "B", "B"}))},
     {"status",    Slate(pluss::make_note({"ok", "ok", "warn", "ok", "warn", "ok"}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
        "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"})));
std::cout << readings;

auto groups = readings.features("sensor_id");
auto out = readings.features({"status"}).ts({.group = groups}).window("2d").count();
std::cout << out;
```

### Output (before)

```text
==============================
           | sensor_id  status
------------------------------
2024-01-01 | A          ok    
2024-01-03 | A          ok    
2024-01-04 | A          warn  
2024-01-02 | B          ok    
2024-01-05 | B          warn  
2024-01-06 | B          ok    
------------------------------
[6 cs x 2 feat]
```

### Output (after)

```text
==============================
     | sensor_id  count_status
------------------------------
0    | A          2           
1    | B          2           
------------------------------
[2 cs x 2 feat]
```

`count()` only tallies how many observations fall in the window — it never
reads `status`'s actual text values, so there's no numeric requirement on it
the way there is for `sum()`/`mean()`/`min()`/`max()`.

## Notes

- See [`sum()`](sum.md) for the full window, closure, and anchoring rules — they
  apply identically here.
- Unlike `sum`/`mean`/`min`/`max`, `count`'s `value_feature` isn't required to be
  numeric.

## See Also

- [`DataFrameTsWindowAccessor::sum`](sum.md),
  [`DataFrameTsWindowAccessor::mean`](mean.md),
  [`DataFrameTsWindowAccessor::min`](min.md),
  [`DataFrameTsWindowAccessor::max`](max.md): the other per-group trailing
  aggregates sharing this same window.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
