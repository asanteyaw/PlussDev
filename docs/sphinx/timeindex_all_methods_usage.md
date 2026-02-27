# TimeIndex: All Methods Usage

This page shows construction and full call patterns for `TimeIndex`.

## Construction: all practical forms

```cpp
#include <pluss/pluss.h>
```

```cpp
// From Timeline
auto t = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z"});
auto idx0 = pluss::make_timeindex(t, true);

// From ISO text
auto idx1 = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z"});

// From epoch nanos
auto idx2 = pluss::make_timeindex(std::vector<std::int64_t>{0, 900000000000}, true);
```

## Core usage patterns

```cpp
// Search/slice
auto key = pluss::make_timeline("2024-01-01T00:10:00Z");
auto pos = idx1.searchsorted(key, false);
auto asof = idx1.asof(key);
auto sel = idx1.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");

// Resample/rolling
auto values = pluss::make_series(std::vector<double>{10,20}, {2});
auto hsum = idx1.resample_sum(values, "H", 1);
auto roll = idx1.rolling_sum(values, pluss::duration_minutes(30));

// Set operations and tz
auto idxu = idx1.union_(idx2);
auto tz = idx1.tz_convert("UTC");
```

## Unit rounding wrappers

`TimeIndex` also exposes wrappers for each unit family:

- `floor_ns`, `ceil_ns`, `round_ns`
- `floor_us`, `ceil_us`, `round_us`
- `floor_ms`, `ceil_ms`, `round_ms`
- `floor_s`, `ceil_s`, `round_s`
- `floor_min`, `ceil_min`, `round_min`
- `floor_h`, `ceil_h`, `round_h`
- `floor_D`, `ceil_D`, `round_D`
- `floor_W`, `ceil_W`, `round_W`

## Canonical runnable coverage

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`

## Full call pattern index from signatures

```{include} ../generated/timeindex_method_call_patterns.md
```
