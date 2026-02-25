# Timeline and TimeIndex Guide

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/codeh/codeh.h>
#include <pluss/codeh/timeline_factory.h>
#include <pluss/codeh/timeindex_factory.h>
```

## Mental model

- `Timeline`: N-D datetime values (0D..3D).
- `TimeIndex`: 1D datetime index ops (search/asof/resample/rolling/alignment).

For exhaustive call forms, use:

- [Timeline: All Methods Usage](timeline_all_methods_usage)
- [TimeIndex: All Methods Usage](timeindex_all_methods_usage)

## Construction quick recipes

```cpp
auto t0 = pluss::moment("2024-01-01T00:00:00Z");
auto t1 = pluss::moment({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto t2 = pluss::from_epoch_nanos(std::vector<std::int64_t>{0, 1'000'000'000}, std::vector<std::int64_t>{2});

auto idx = pluss::make_timeindex({
  "2024-01-01T00:00:00Z",
  "2024-01-01T00:15:00Z",
  "2024-01-01T00:45:00Z",
  "2024-01-01T01:05:00Z",
});
```

## High-value workflows

```cpp
// Timeline arithmetic and extraction
auto next = t0.add_duration(pluss::duration_minutes(5));
auto years = next.year();
auto fmt = next.strftime("%Y-%m-%d %H:%M:%S");

// TimeIndex resample and rolling
auto values = pluss::value(std::vector<double>{10,20,30,40}, {4});
auto hourly_sum = idx.resample_agg(values, "H", "sum", 1);
auto rolling = idx.rolling_agg(values, pluss::duration_minutes(45), "mean");

// search/slice
auto key = pluss::moment("2024-01-01T00:40:00Z");
auto insert_pos = idx.searchsorted(key, false);
auto asof_ix = idx.asof(key);
auto slice_ix = idx.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");
```

## Demos

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`
