<!-- pluss-hand-authored -->

# DataFrameTsWindowAccessor::sum

[Back to DataFrameTsWindowAccessor API Reference](../../dataframe_ts_window_accessor_api_reference)

## Signature

```cpp
DataFrameTsWindowAccessor::sum(std::string_view value_feature = {}) const -> DataFrame
```

## What It Does

Computes, **once per group**, the sum of `value_feature` over the trailing
duration-length window ending at that group's own most recent time-axis value (or
an explicit `TsOptions.anchor`, if one was set). This is a per-group trailing
aggregate — not a resample-style bucketing — so the result has exactly one row
per distinct group, not one row per original observation.

## Parameters

**value_feature** (`std::string_view`, default empty)

The feature to sum. May be omitted only if exactly one feature is currently bound
on the accessor, in which case that feature is used automatically.

## Returns

`DataFrame`
    One row per distinct group, with a group-key column (or columns) and a
    `"sum_" + value_feature` column.

## Examples

Example 1. Sum each sensor's readings over the trailing 2 days ending at that
sensor's own last reading.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sensor_id", Thread(pluss::make_note({"A", "A", "A", "B", "B", "B"}))},
     {"value",     Thread(pluss::make_series({10.0, 20.0, 15.0, 5.0, 8.0, 12.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
        "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"})));
std::cout << readings;

auto groups = readings.features("sensor_id");
auto out = readings.features({"value"}).ts({.group = groups}).window("2d").sum();
std::cout << out;
```

### Output (before)

```text
=============================
           | sensor_id  value
-----------------------------
2024-01-01 | A          10   
2024-01-03 | A          20   
2024-01-04 | A          15   
2024-01-02 | B          5    
2024-01-05 | B          8    
2024-01-06 | B          12   
-----------------------------
[6 cs x 2 feat]
```

### Output (after)

```text
===========================
     | sensor_id  sum_value
---------------------------
0    | A          35       
1    | B          20       
---------------------------
[2 cs x 2 feat]
```

Sensor `A`'s most recent reading is `2024-01-04`; the trailing 2-day window
`(2024-01-02, 2024-01-04]` includes `2024-01-03` (`20`) and `2024-01-04` (`15`),
but excludes `2024-01-01` (too early), so `sum_value = 35`. Sensor `B`'s most
recent reading is `2024-01-06`; the window `(2024-01-04, 2024-01-06]` includes
`2024-01-05` (`8`) and `2024-01-06` (`12`), giving `sum_value = 20`. Groups appear
in the order they first occur in the source (`A` before `B`).

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
auto out = readings.features({"temp", "humidity"}).ts({.group = groups}).window("2d").sum("humidity");
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
     | sensor_id  sum_humidity
------------------------------
0    | A          82          
1    | B          105         
------------------------------
[2 cs x 2 feat]
```

With two features bound (`temp` and `humidity`), `value_feature` can no longer
be inferred — naming `"humidity"` explicitly picks it out and leaves `temp`
untouched by the aggregate.

Example 3. Call `.window(...)` without first setting `TsOptions.group`, to
see it throw immediately — before `sum()` is even called.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({10.0, 20.0, 15.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"})));

try {
  auto windowed = readings.ts().window("2d");
} catch (const std::exception& e) {
  std::cout << std::string(e.what()).substr(0, 56) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::window: TsOptions.group is required
```

The check happens as soon as `.window(...)` builds the
`DataFrameTsWindowAccessor` — `sum()` (or any other aggregate) never even
gets a chance to run without a group already set via
`df.features({...}).ts({.group = df.features("group_col")})`.

## Notes

- Requires `TsOptions.group` to already be set (via
  `df.features({...}).ts({.group = df.features("group_col")})`, extracting the
  group column *before* narrowing to the numeric value feature, exactly as shown
  above) — the accessor throws at construction otherwise.
- The window is right-closed by default (`WindowClosure::Right`, set via
  [`window()`](../dataframe-ts/window.md)'s `closure` parameter): it excludes the
  anchor-minus-duration boundary itself and includes the anchor.
- With no explicit `TsOptions.anchor`, each group's own anchor is that group's
  *own* maximum time-axis value — different groups can anchor to different
  instants.

## See Also

- [`DataFrameTsWindowAccessor::mean`](mean.md),
  [`DataFrameTsWindowAccessor::min`](min.md),
  [`DataFrameTsWindowAccessor::max`](max.md),
  [`DataFrameTsWindowAccessor::count`](count.md): the other per-group trailing
  aggregates sharing this same window.
- [`DataFrameTsAccessor::window`](../dataframe-ts/window.md): the entry point that
  produces this accessor.
- [`DataFrameTsAccessor::grouped`](../dataframe-ts/grouped.md): a row-aligned
  (not one-row-per-group) grouped rolling statistic, a different shape entirely.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
</div>
