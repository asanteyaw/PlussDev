# Timeline and TimeIndex Guide (Usage-First)

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/codeh/codeh.h>
#include <pluss/codeh/timeline_factory.h>
#include <pluss/codeh/timeindex_factory.h>
```

## How to read this page

- Real use-case snippets first.
- Comments explain when to use each method family.
- Full current call-pattern coverage for both `Timeline` and `TimeIndex` is included at the end.

## Timeline Construction: All Practical Entry Points

```cpp
// Scalar / 1D / ND from ISO-like text
auto t0 = pluss::make_timeline("2024-01-01T00:00:00Z");
auto t1 = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto t2 = pluss::make_timeline({{"2024-02-01T00:00:00Z", "2024-02-01T01:00:00Z"}});

// Explicit flat + shape
std::vector<std::string_view> flat{"2024-03-01T00:00:00Z", "2024-03-01T01:00:00Z"};
auto t3 = pluss::make_timeline(flat, std::vector<std::int64_t>{2});

// Epoch nanos -> Timeline
auto t4 = pluss::timeline::from_epoch(0);
auto t5 = pluss::timeline::from_epoch(std::vector<std::int64_t>{0, 1'000'000'000}, std::vector<std::int64_t>{2});
```

## Timeline Arithmetic, Compare, and Field Extraction

```cpp
auto base = pluss::make_timeline("2024-01-01T00:00:00Z");
auto d5m = pluss::duration_minutes(5);

// arithmetic
auto next = base.add_duration(d5m);
auto back = next.sub_duration(d5m);
auto dt = next.diff(base);

// comparisons -> Series mask
auto gt = next.gt(base);
auto eq = next.eq(base);

// date-time fields -> Series
auto yr = next.year();
auto mon = next.month();
auto dow = next.weekday();
```

Helpful note:

- Field aliases (`year/y/yr/years`, `weekday/dayofweek/dow`, etc.) are equivalent wrappers; use one style consistently in your codebase.

## Timeline Formatting, Timezone, and Rounding

```cpp
auto t = pluss::make_timeline({"2024-06-15T12:30:10Z", "2024-12-31T23:59:59Z"});

// format to Note
auto fmt = t.strftime("%Y-%m-%d %H:%M:%S");
auto iso = t.to_note_iso();

// timezone
auto utc = t.to_utc();
auto nyc = t.tz_convert("America/New_York");

// rounding
auto day = t.floor("D");
auto hour = t.ceil("h");
auto rounded = t.round("min");
```

## Timeline Legacy/Scalar Helpers (Still Useful)

```cpp
auto s = pluss::codeh::Timeline::createFromDateTimeInts(2024, 2, 29, 12, 34, 56);

// scalar breakdown
int y = s.year_scalar();
int m = s.month_scalar();
int d = s.day_scalar();

// immutable setters / arithmetic
auto s2 = s.set_year(2030).set_month(12).set_day(15);
auto s3 = s.add_days(7).add_hours(3);

// conversions
auto as_series = s.to_Series();
auto as_text = s.to_string();
```

## TimeIndex Construction: All Practical Entry Points

```cpp
// from Timeline
auto axis = pluss::make_timeline({
  "2024-01-01T00:00:00Z",
  "2024-01-01T00:15:00Z",
  "2024-01-01T00:45:00Z",
  "2024-01-01T01:05:00Z"
});
auto idx0 = pluss::make_timeindex(axis, true);

// from text directly
auto idx1 = pluss::make_timeindex({
  "2024-01-01T00:00:00Z",
  "2024-01-01T00:15:00Z",
  "2024-01-01T00:45:00Z",
  "2024-01-01T01:05:00Z"
});

// from epoch nanos
auto idx2 = pluss::make_timeindex(std::vector<std::int64_t>{0, 900000000000, 1800000000000}, true);
```

## TimeIndex Search/Slice/Asof

```cpp
auto key = pluss::make_timeline("2024-01-01T00:40:00Z");

auto left_insert = idx1.searchsorted(key, false);
auto right_insert = idx1.searchsorted(key, true);
auto asof_ix = idx1.asof(key);

auto by_time = idx1.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");
auto by_key = idx1.slice(pluss::make_timeline("2024-01-01T00:00:00Z"), pluss::make_timeline("2024-01-01T01:00:00Z"));
```

## TimeIndex Resample and Rolling

```cpp
auto vals = pluss::make_series(std::vector<double>{10,20,30,40}, {4});

// direct resample ops
auto c = idx1.resample_count("H", 1);
auto s = idx1.resample_sum(vals, "H", 1);
auto m = idx1.resample_mean(vals, "H", 1);
auto ohlc = idx1.resample_ohlc(vals, "H", 1);

// generic aggregator entrypoint
auto via_agg = idx1.resample_agg(vals, "H", "max", 1);

// rolling windows
auto win = pluss::duration_minutes(45);
auto rc = idx1.rolling_count(win);
auto rs = idx1.rolling_sum(vals, win);
auto rm = idx1.rolling_agg(vals, win, "mean");
```

Helpful note:

- `resample_agg`/`rolling_agg` are best when driving behavior from config or user input (`"sum"`, `"mean"`, ...).

## TimeIndex Set Ops, Join, Align, Frequency

```cpp
auto right = pluss::make_timeindex({
  "2024-01-01T00:05:00Z",
  "2024-01-01T00:35:00Z",
  "2024-01-01T01:05:00Z"
});

auto uni = idx1.union_(right);
auto inter = idx1.intersection(right);
auto diff = idx1.difference(right);

auto joined = idx1.join_asof(right, pluss::codeh::TimeIndex::AsofDirection::Backward, nullptr, true);
auto aligned = idx1.align_union(right);

auto freq = idx1.freq();
auto monotonic = idx1.is_monotonic_increasing();
```

## Timezone and Rounding Wrappers on TimeIndex

```cpp
auto tz = idx1.tz_convert("UTC");
auto loc = idx1.tz_localize("UTC", 0, "raise");

// unit wrappers
auto d = idx1.floor_D();
auto h = idx1.round_h();
auto m = idx1.ceil_min();
```
