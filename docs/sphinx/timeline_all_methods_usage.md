# Timeline: All Methods Usage

This page shows construction and full call patterns for `Timeline`.

## Construction: all practical forms

```cpp
#include <pluss/pluss.h>
```

```cpp
// Scalar / 1D / ND from timestamp text
auto t0 = pluss::moment("2024-01-01T00:00:00Z");
auto t1 = pluss::moment({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto t2 = pluss::moment({{"2024-02-01T00:00:00Z", "2024-02-01T01:00:00Z"}});

// Explicit flat + shape
std::vector<std::string_view> flat{"2024-03-01T00:00:00Z", "2024-03-01T01:00:00Z"};
auto t3 = pluss::make_timeline(flat, std::vector<std::int64_t>{2});

// Epoch nanos
auto t4 = pluss::from_epoch_nanos(0);
auto t5 = pluss::from_epoch_nanos(std::vector<std::int64_t>{0, 1'000'000'000}, std::vector<std::int64_t>{2});
```

## Core usage patterns

```cpp
// Arithmetic and compare
auto d = pluss::duration_minutes(5);
auto t_next = t0.add_duration(d);
auto dt = t_next.diff(t0);
auto mask = t_next.gt(t0);

// Rounding/timezone/format
auto day = t_next.floor("D");
auto utc = t_next.to_utc();
auto fmt = t_next.strftime("%Y-%m-%d %H:%M:%S");

// Date-time fields
auto yy = t_next.year();
auto wd = t_next.weekday();
```

## Alias families

- Date-time field aliases (`year/y/yr/years`, `month/mon/mo/...`, `weekday/dayofweek/dow`, etc.) are interchangeable wrappers over the same operation.
- `to_note_*` methods are generated formatting aliases; each call is simply `t.to_note_xyz()` with no arguments.

## Canonical runnable coverage

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`

## Full call pattern index from signatures

```{include} ../generated/timeline_method_call_patterns.md
```
