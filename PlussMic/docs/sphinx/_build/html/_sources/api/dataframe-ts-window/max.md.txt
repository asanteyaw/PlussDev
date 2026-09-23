<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d54dc645054a -->

# DataFrameTsWindowAccessor::max

[Back to DataFrameTsWindowAccessor API Reference](../../dataframe_ts_window_accessor_api_reference)

## Signature

```cpp
DataFrameTsWindowAccessor::max(std::string_view value_feature = {}) const -> DataFrame
```

## What It Does

Computes, **once per group**, the maximum of `value_feature` over the trailing
duration-length window ending at that group's own most recent time-axis value (or
an explicit `TsOptions.anchor`, if one was set) — the same per-group trailing
window [`sum()`](sum.md) uses, with `Max` in place of `Sum`. See
[`sum()`](sum.md) for the full setup requirements (`TsOptions.group`, window
closure, anchoring).

## Parameters

**value_feature** (`std::string_view`, default empty)

The feature to maximize. May be omitted only if exactly one feature is currently
bound on the accessor.

## Returns

`DataFrame`
    One row per distinct group, with a group-key column (or columns) and a
    `"max_" + value_feature` column.

## Examples

Example 1. The maximum of each sensor's readings over the trailing 2 days ending
at that sensor's own last reading — the same dataset as [`sum()`](sum.md).

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sensor_id", Thread(pluss::make_note({"A", "A", "A", "B", "B", "B"}))},
     {"value",     Thread(pluss::make_series({10.0, 20.0, 15.0, 5.0, 8.0, 12.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
        "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"})));

auto groups = readings.features("sensor_id");
auto out = readings.features({"value"}).ts({.group = groups}).window("2d").max();
std::cout << out;
```

### Output

```text
===========================
     | sensor_id  max_value
---------------------------
0    | A          20       
1    | B          12       
---------------------------
[2 cs x 2 feat]
```

Sensor `A`'s trailing window holds `(20, 15)`, so the maximum is `20`; sensor
`B`'s holds `(8, 12)`, so the maximum is `12`.

Example 2. Pass `value_feature` explicitly when more than one feature is bound
on the accessor.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sensor_id", Thread(pluss::make_note({"A", "A", "B", "B"}))},
     {"temp",      Thread(pluss::make_series({10.0, 20.0, 5.0, 8.0}))},
     {"humidity",  Thread(pluss::make_series({40.0, 42.0, 55.0, 50.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z",
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})));
std::cout << readings;

auto groups = readings.features("sensor_id");
auto out = readings.features({"temp", "humidity"}).ts({.group = groups}).window("2d").max("humidity");
std::cout << out;
```

### Output (before)

```text
======================================
           | sensor_id  temp  humidity
--------------------------------------
2024-01-01 | A          10    40      
2024-01-02 | A          20    42      
2024-01-01 | B          5     55      
2024-01-02 | B          8     50      
--------------------------------------
[4 cs x 3 feat]
```

### Output (after)

```text
==============================
     | sensor_id  max_humidity
------------------------------
0    | A          42          
1    | B          55          
------------------------------
[2 cs x 2 feat]
```

With two features bound (`temp` and `humidity`), `value_feature` can no longer
be inferred — naming `"humidity"` explicitly picks it out and leaves `temp`
untouched by the aggregate.

## Notes

- See [`sum()`](sum.md) for the full window, closure, and anchoring rules — they
  apply identically here.
- A group whose window ends up with zero observations produces `NULL`.

## See Also

- [`DataFrameTsWindowAccessor::sum`](sum.md),
  [`DataFrameTsWindowAccessor::mean`](mean.md),
  [`DataFrameTsWindowAccessor::min`](min.md),
  [`DataFrameTsWindowAccessor::count`](count.md): the other per-group trailing
  aggregates sharing this same window.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
