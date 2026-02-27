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
auto t0 = pluss::moment("2024-01-01T00:00:00Z");
auto t1 = pluss::moment({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto t2 = pluss::moment({{"2024-02-01T00:00:00Z", "2024-02-01T01:00:00Z"}});

// Explicit flat + shape
std::vector<std::string_view> flat{"2024-03-01T00:00:00Z", "2024-03-01T01:00:00Z"};
auto t3 = pluss::make_timeline(flat, std::vector<std::int64_t>{2});

// Epoch nanos -> Timeline
auto t4 = pluss::from_epoch_nanos(0);
auto t5 = pluss::from_epoch_nanos(std::vector<std::int64_t>{0, 1'000'000'000}, std::vector<std::int64_t>{2});
```

## Timeline Arithmetic, Compare, and Field Extraction

```cpp
auto base = pluss::moment("2024-01-01T00:00:00Z");
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
auto t = pluss::moment({"2024-06-15T12:30:10Z", "2024-12-31T23:59:59Z"});

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
auto axis = pluss::moment({
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
auto key = pluss::moment("2024-01-01T00:40:00Z");

auto left_insert = idx1.searchsorted(key, false);
auto right_insert = idx1.searchsorted(key, true);
auto asof_ix = idx1.asof(key);

auto by_time = idx1.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");
auto by_key = idx1.slice(pluss::moment("2024-01-01T00:00:00Z"), pluss::moment("2024-01-01T01:00:00Z"));
```

## TimeIndex Resample and Rolling

```cpp
auto vals = pluss::value(std::vector<double>{10,20,30,40}, {4});

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

## Implemented Timeline API: Signatures + Real Calls

Only methods with concrete runtime routes in `CodeH/compat/Dispatch.cpp` are documented here.
Signature convention: `self` is the receiver object for instance methods.

# Complete Timeline Method Reference: Signatures + Real Calls

```cpp
auto t = pluss::moment({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto other_t = pluss::moment({"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"});
auto dur = pluss::duration_minutes(5);
auto mask = pluss::Series::ones({2}, pluss::ScalarType::Bool);
auto series_vals = pluss::value(std::vector<double>{1.0, 2.0}, {2});
auto year_col = pluss::value(std::vector<std::int64_t>{2024,2024}, {2});
auto month_col = pluss::value(std::vector<std::int64_t>{1,1}, {2});
auto day_col = pluss::value(std::vector<std::int64_t>{10,11}, {2});
auto hour_col = pluss::value(std::vector<std::int64_t>{8,9}, {2});
auto minute_col = pluss::value(std::vector<std::int64_t>{30,45}, {2});
auto second_col = pluss::value(std::vector<std::int64_t>{0,0}, {2});
std::vector<pluss::codeh::Timeline> others{other_t};
```

- Signature:
  `int year_scalar(self) const`
- Explanation:
  Applies `year_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.year_scalar();`
- Signature:
  `int month_scalar(self) const`
- Explanation:
  Applies `month_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.month_scalar();`
- Signature:
  `int day_scalar(self) const`
- Explanation:
  Applies `day_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.day_scalar();`
- Signature:
  `int hour_scalar(self) const`
- Explanation:
  Applies `hour_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.hour_scalar();`
- Signature:
  `int minute_scalar(self) const`
- Explanation:
  Applies `minute_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.minute_scalar();`
- Signature:
  `int second_scalar(self) const`
- Explanation:
  Applies `second_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.second_scalar();`
- Signature:
  `int day_of_year_scalar(self) const`
- Explanation:
  Applies `day_of_year_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.day_of_year_scalar();`
- Signature:
  `int quarter_scalar(self) const`
- Explanation:
  Applies `quarter_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.quarter_scalar();`
- Signature:
  `int week_of_year_scalar(self) const`
- Explanation:
  Applies `week_of_year_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.week_of_year_scalar();`
- Signature:
  `bool is_leap_year_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_leap_year_scalar();`
- Signature:
  `bool is_month_start_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_start_scalar();`
- Signature:
  `bool is_month_end_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_end_scalar();`
- Signature:
  `bool is_quarter_start_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_start_scalar();`
- Signature:
  `bool is_quarter_end_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_end_scalar();`
- Signature:
  `bool is_weekend_scalar(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_weekend_scalar();`
- Signature:
  `std::string weekday_name(self, bool abbreviated) const`
- Explanation:
  Applies `weekday_name` to `self`. Arguments: `bool abbreviated`.
- Example:
  `auto out = t.weekday_name(true);`
- Signature:
  `std::string month_name(self, bool abbreviated) const`
- Explanation:
  Applies `month_name` to `self`. Arguments: `bool abbreviated`.
- Example:
  `auto out = t.month_name(true);`
- Signature:
  `Timeline set_year(self, int year) const`
- Explanation:
  Applies `set_year` to `self`. Arguments: `int year`.
- Example:
  `auto out = t.set_year(2025);`
- Signature:
  `Timeline set_month(self, int month) const`
- Explanation:
  Applies `set_month` to `self`. Arguments: `int month`.
- Example:
  `auto out = t.set_month(12);`
- Signature:
  `Timeline set_day(self, int day) const`
- Explanation:
  Applies `set_day` to `self`. Arguments: `int day`.
- Example:
  `auto out = t.set_day(15);`
- Signature:
  `Timeline set_hour(self, int hour) const`
- Explanation:
  Applies `set_hour` to `self`. Arguments: `int hour`.
- Example:
  `auto out = t.set_hour(9);`
- Signature:
  `Timeline set_minute(self, int minute) const`
- Explanation:
  Applies `set_minute` to `self`. Arguments: `int minute`.
- Example:
  `auto out = t.set_minute(30);`
- Signature:
  `Timeline set_second(self, int second) const`
- Explanation:
  Applies `set_second` to `self`. Arguments: `int second`.
- Example:
  `auto out = t.set_second(45);`
- Signature:
  `std::string format(self, const std::string& format_str) const`
- Explanation:
  Formats/converts `self` to a textual representation. Arguments: `const std::string& format_str`.
- Example:
  `auto out = t.format("%Y-%m-%d");`
- Signature:
  `std::string toISOString(self) const`
- Explanation:
  Applies `toISOString` to `self`. No additional arguments.
- Example:
  `auto out = t.toISOString();`
- Signature:
  `std::string to_string(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = t.to_string();`
- Signature:
  `pluss::codem::Series to_Series(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = t.to_Series();`
- Signature:
  `Timeline add_years(self, int years) const`
- Explanation:
  Applies `add_years` to `self`. Arguments: `int years`.
- Example:
  `auto out = t.add_years(2);`
- Signature:
  `Timeline add_months(self, int months) const`
- Explanation:
  Applies `add_months` to `self`. Arguments: `int months`.
- Example:
  `auto out = t.add_months(3);`
- Signature:
  `Timeline add_days(self, int days) const`
- Explanation:
  Applies `add_days` to `self`. Arguments: `int days`.
- Example:
  `auto out = t.add_days(10);`
- Signature:
  `Timeline add_hours(self, int hours) const`
- Explanation:
  Applies `add_hours` to `self`. Arguments: `int hours`.
- Example:
  `auto out = t.add_hours(6);`
- Signature:
  `Timeline add_minutes(self, int minutes) const`
- Explanation:
  Applies `add_minutes` to `self`. Arguments: `int minutes`.
- Example:
  `auto out = t.add_minutes(45);`
- Signature:
  `Timeline add_seconds(self, int seconds) const`
- Explanation:
  Applies `add_seconds` to `self`. Arguments: `int seconds`.
- Example:
  `auto out = t.add_seconds(30);`
- Signature:
  `int years_between(self, const Timeline& other) const`
- Explanation:
  Applies `years_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.years_between(other_t);`
- Signature:
  `int months_between(self, const Timeline& other) const`
- Explanation:
  Applies `months_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.months_between(other_t);`
- Signature:
  `int days_between(self, const Timeline& other) const`
- Explanation:
  Applies `days_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.days_between(other_t);`
- Signature:
  `int hours_between(self, const Timeline& other) const`
- Explanation:
  Applies `hours_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.hours_between(other_t);`
- Signature:
  `int minutes_between(self, const Timeline& other) const`
- Explanation:
  Applies `minutes_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.minutes_between(other_t);`
- Signature:
  `int seconds_between(self, const Timeline& other) const`
- Explanation:
  Applies `seconds_between` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.seconds_between(other_t);`
- Signature:
  `Timeline concat(self, const Timeline& other) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.concat(other_t);`
- Signature:
  `Timeline concat(self, const std::vector<Timeline>& others) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<Timeline>& others`.
- Example:
  `auto out = t.concat(others);`
- Signature:
  `Timeline repeat(self, std::int64_t times) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t times`.
- Example:
  `auto out = t.repeat(3);`
- Signature:
  `Timeline empty_like(self) const`
- Explanation:
  Applies `empty_like` to `self`. No additional arguments.
- Example:
  `auto out = t.empty_like();`
- Signature:
  `Timeline fill_like(self, int year, int month, int day) const`
- Explanation:
  Applies `fill_like` to `self`. Arguments: `int year, int month, int day`.
- Example:
  `auto out = t.fill_like(2024, 12, 25);`
- Signature:
  `Timeline fill_like(self, const std::string& date_str) const`
- Explanation:
  Applies `fill_like` to `self`. Arguments: `const std::string& date_str`.
- Example:
  `auto out = t.fill_like("2024-12-25T00:00:00Z");`
- Signature:
  `Timeline shift(self, int periods, std::string_view freq) const`
- Explanation:
  Applies `shift` to `self`. Arguments: `int periods, std::string_view freq`.
- Example:
  `auto out = t.shift(1, "D");`
- Signature:
  `Timeline to_period(self, std::string_view freq) const`
- Explanation:
  Converts between related PlussQit data types. Arguments: `std::string_view freq`.
- Example:
  `auto out = t.to_period("D");`
- Signature:
  `TimeIndex to_timeindex(self, bool require_sorted) const`
- Explanation:
  Builds a `TimeIndex` view from timeline values (optionally enforcing sortedness). Arguments: `bool require_sorted`.
- Example:
  `auto out = t.to_timeindex(true);`
- Signature:
  `pluss::codem::Series epoch_nanos(self) const`
- Explanation:
  Applies `epoch_nanos` to `self`. No additional arguments.
- Example:
  `auto out = t.epoch_nanos();`
- Signature:
  `pluss::codem::Series epoch_micros(self) const`
- Explanation:
  Applies `epoch_micros` to `self`. No additional arguments.
- Example:
  `auto out = t.epoch_micros();`
- Signature:
  `pluss::codem::Series epoch_millis(self) const`
- Explanation:
  Applies `epoch_millis` to `self`. No additional arguments.
- Example:
  `auto out = t.epoch_millis();`
- Signature:
  `pluss::codem::Series epoch_seconds(self) const`
- Explanation:
  Applies `epoch_seconds` to `self`. No additional arguments.
- Example:
  `auto out = t.epoch_seconds();`
- Signature:
  `Timeline::now()`
- Explanation:
  Static constructor that returns a timeline anchored to `now`. No additional arguments.
- Example:
  `auto out = pluss::codeh::Timeline::now();`
- Signature:
  `Timeline::today()`
- Explanation:
  Static constructor that returns a timeline anchored to `today`. No additional arguments.
- Example:
  `auto out = pluss::codeh::Timeline::today();`
- Signature:
  `Timeline::yesterday()`
- Explanation:
  Static constructor that returns a timeline anchored to `yesterday`. No additional arguments.
- Example:
  `auto out = pluss::codeh::Timeline::yesterday();`
- Signature:
  `Timeline::tomorrow()`
- Explanation:
  Static constructor that returns a timeline anchored to `tomorrow`. No additional arguments.
- Example:
  `auto out = pluss::codeh::Timeline::tomorrow();`
- Signature:
  `Timeline::fromTimestamp(std::int64_t timestamp_seconds)`
- Explanation:
  Static helper/factory method. Arguments: `std::int64_t timestamp_seconds`.
- Example:
  `auto out = pluss::codeh::Timeline::fromTimestamp(1704067200);`
- Signature:
  `Timeline::fromSeries(const pluss::codem::Series& series)`
- Explanation:
  Static helper/factory method. Arguments: `const pluss::codem::Series& series`.
- Example:
  `auto out = pluss::codeh::Timeline::fromSeries(series_vals);`
- Signature:
  `Timeline::createFromDateInts(int year, int month, int day)`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `int year, int month, int day`.
- Example:
  `auto out = pluss::codeh::Timeline::createFromDateInts(2024, 1, 10);`
- Signature:
  `Timeline::createFromTimeInts(int hour, int minute, int second)`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `int hour, int minute, int second`.
- Example:
  `auto out = pluss::codeh::Timeline::createFromTimeInts(8, 30, 0);`
- Signature:
  `Timeline add_duration(self, const Duration& d) const`
- Explanation:
  Applies `add_duration` to `self`. Arguments: `const Duration& d`.
- Example:
  `auto out = t.add_duration(dur);`
- Signature:
  `Timeline sub_duration(self, const Duration& d) const`
- Explanation:
  Applies `sub_duration` to `self`. Arguments: `const Duration& d`.
- Example:
  `auto out = t.sub_duration(dur);`
- Signature:
  `Duration diff(self, const Timeline& other) const`
- Explanation:
  Applies `diff` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.diff(other_t);`
- Signature:
  `pluss::codem::Series eq(self, const Timeline& other) const`
- Explanation:
  Applies `eq` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.eq(other_t);`
- Signature:
  `pluss::codem::Series ne(self, const Timeline& other) const`
- Explanation:
  Applies `ne` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.ne(other_t);`
- Signature:
  `pluss::codem::Series lt(self, const Timeline& other) const`
- Explanation:
  Applies `lt` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.lt(other_t);`
- Signature:
  `pluss::codem::Series le(self, const Timeline& other) const`
- Explanation:
  Applies `le` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.le(other_t);`
- Signature:
  `pluss::codem::Series gt(self, const Timeline& other) const`
- Explanation:
  Applies `gt` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.gt(other_t);`
- Signature:
  `pluss::codem::Series ge(self, const Timeline& other) const`
- Explanation:
  Applies `ge` to `self`. Arguments: `const Timeline& other`.
- Example:
  `auto out = t.ge(other_t);`
- Signature:
  `pluss::codem::Series isnat(self) const`
- Explanation:
  Applies `isnat` to `self`. No additional arguments.
- Example:
  `auto out = t.isnat();`
- Signature:
  `pluss::codem::Series isvalid(self) const`
- Explanation:
  Applies `isvalid` to `self`. No additional arguments.
- Example:
  `auto out = t.isvalid();`
- Signature:
  `pluss::codem::Series argsort(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = t.argsort();`
- Signature:
  `bool isnat_scalar(self) const`
- Explanation:
  Applies `isnat_scalar` to `self`. No additional arguments.
- Example:
  `auto out = t.isnat_scalar();`
- Signature:
  `std::string isoformat(self) const`
- Explanation:
  Applies `isoformat` to `self`. No additional arguments.
- Example:
  `auto out = t.isoformat();`
- Signature:
  `pluss::coden::Note strftime(self, std::string_view fmt) const`
- Explanation:
  Formats/converts `self` to a textual representation. Arguments: `std::string_view fmt`.
- Example:
  `auto out = t.strftime("%Y-%m-%d");`
- Signature:
  `pluss::coden::Note to_note_iso(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_iso();`
- Signature:
  `pluss::coden::Note weekday_name(self) const`
- Explanation:
  Applies `weekday_name` to `self`. No additional arguments.
- Example:
  `auto out = t.weekday_name();`
- Signature:
  `pluss::coden::Note month_name(self) const`
- Explanation:
  Applies `month_name` to `self`. No additional arguments.
- Example:
  `auto out = t.month_name();`
- Signature:
  `Timeline floor(self, std::string_view unit) const`
- Explanation:
  Applies `floor` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = t.floor("D");`
- Signature:
  `Timeline ceil(self, std::string_view unit) const`
- Explanation:
  Applies `ceil` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = t.ceil("D");`
- Signature:
  `Timeline round(self, std::string_view unit) const`
- Explanation:
  Applies `round` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = t.round("D");`
- Signature:
  `Timeline normalize(self) const`
- Explanation:
  Applies `normalize` to `self`. No additional arguments.
- Example:
  `auto out = t.normalize();`
- Signature:
  `Timeline date(self) const`
- Explanation:
  Applies `date` to `self`. No additional arguments.
- Example:
  `auto out = t.date();`
- Signature:
  `Timeline time(self) const`
- Explanation:
  Applies `time` to `self`. No additional arguments.
- Example:
  `auto out = t.time();`
- Signature:
  `Timeline tz_convert(self, std::string_view tz_name) const`
- Explanation:
  Applies `tz_convert` to `self`. Arguments: `std::string_view tz_name`.
- Example:
  `auto out = t.tz_convert("UTC");`
- Signature:
  `Timeline tz_localize(self, std::string_view tz_name, int fold, std::string_view nonexistent) const`
- Explanation:
  Applies `tz_localize` to `self`. Arguments: `std::string_view tz_name, int fold, std::string_view nonexistent`.
- Example:
  `auto out = t.tz_localize("UTC", 1, "raise");`
- Signature:
  `Timeline to_utc(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = t.to_utc();`
- Signature:
  `Timeline min(self) const`
- Explanation:
  Applies `min` to `self`. No additional arguments.
- Example:
  `auto out = t.min();`
- Signature:
  `Timeline max(self) const`
- Explanation:
  Applies `max` to `self`. No additional arguments.
- Example:
  `auto out = t.max();`
- Signature:
  `Timeline sort(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = t.sort();`
- Signature:
  `Timeline unique(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = t.unique();`
- Signature:
  `Timeline drop_duplicates(self) const`
- Explanation:
  Applies `drop_duplicates` to `self`. No additional arguments.
- Example:
  `auto out = t.drop_duplicates();`
- Signature:
  `Timeline fill_nat(self, const Timeline& value) const`
- Explanation:
  Applies `fill_nat` to `self`. Arguments: `const Timeline& value`.
- Example:
  `auto out = t.fill_nat(other_t);`
- Signature:
  `Timeline where(self, const pluss::codem::Series& mask, const Timeline& other) const`
- Explanation:
  Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const pluss::codem::Series& mask, const Timeline& other`.
- Example:
  `auto out = t.where(mask, other_t);`
- Signature:
  `Timeline add_nanos(self, std::int64_t nanos) const`
- Explanation:
  Applies `add_nanos` to `self`. Arguments: `std::int64_t nanos`.
- Example:
  `auto out = t.add_nanos(1'000'000);`
- Signature:
  `pluss::codem::Series searchsorted_left(self, const Timeline& values) const`
- Explanation:
  Applies `searchsorted_left` to `self`. Arguments: `const Timeline& values`.
- Example:
  `auto out = t.searchsorted_left(other_t);`
- Signature:
  `pluss::codem::Series searchsorted_right(self, const Timeline& values) const`
- Explanation:
  Applies `searchsorted_right` to `self`. Arguments: `const Timeline& values`.
- Example:
  `auto out = t.searchsorted_right(other_t);`
- Signature:
  `std::vector<std::int64_t> to_epoch_nanos_flat(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = t.to_epoch_nanos_flat();`
- Signature:
  `pluss::codem::Series year(self) const`
- Explanation:
  Applies `year` to `self`. No additional arguments.
- Example:
  `auto out = t.year();`
- Signature:
  `pluss::codem::Series y(self) const`
- Explanation:
  Applies `y` to `self`. No additional arguments.
- Example:
  `auto out = t.y();`
- Signature:
  `pluss::codem::Series yr(self) const`
- Explanation:
  Applies `yr` to `self`. No additional arguments.
- Example:
  `auto out = t.yr();`
- Signature:
  `pluss::codem::Series years(self) const`
- Explanation:
  Applies `years` to `self`. No additional arguments.
- Example:
  `auto out = t.years();`
- Signature:
  `pluss::codem::Series year_(self) const`
- Explanation:
  In-place variant of `year`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.year_();`
- Signature:
  `pluss::codem::Series month(self) const`
- Explanation:
  Applies `month` to `self`. No additional arguments.
- Example:
  `auto out = t.month();`
- Signature:
  `pluss::codem::Series mon(self) const`
- Explanation:
  Applies `mon` to `self`. No additional arguments.
- Example:
  `auto out = t.mon();`
- Signature:
  `pluss::codem::Series mo(self) const`
- Explanation:
  Applies `mo` to `self`. No additional arguments.
- Example:
  `auto out = t.mo();`
- Signature:
  `pluss::codem::Series months(self) const`
- Explanation:
  Applies `months` to `self`. No additional arguments.
- Example:
  `auto out = t.months();`
- Signature:
  `pluss::codem::Series month_(self) const`
- Explanation:
  In-place variant of `month`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.month_();`
- Signature:
  `pluss::codem::Series day(self) const`
- Explanation:
  Applies `day` to `self`. No additional arguments.
- Example:
  `auto out = t.day();`
- Signature:
  `pluss::codem::Series d(self) const`
- Explanation:
  Applies `d` to `self`. No additional arguments.
- Example:
  `auto out = t.d();`
- Signature:
  `pluss::codem::Series dom(self) const`
- Explanation:
  Applies `dom` to `self`. No additional arguments.
- Example:
  `auto out = t.dom();`
- Signature:
  `pluss::codem::Series day_(self) const`
- Explanation:
  In-place variant of `day`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.day_();`
- Signature:
  `pluss::codem::Series days(self) const`
- Explanation:
  Applies `days` to `self`. No additional arguments.
- Example:
  `auto out = t.days();`
- Signature:
  `pluss::codem::Series hour(self) const`
- Explanation:
  Applies `hour` to `self`. No additional arguments.
- Example:
  `auto out = t.hour();`
- Signature:
  `pluss::codem::Series h(self) const`
- Explanation:
  Applies `h` to `self`. No additional arguments.
- Example:
  `auto out = t.h();`
- Signature:
  `pluss::codem::Series hr(self) const`
- Explanation:
  Applies `hr` to `self`. No additional arguments.
- Example:
  `auto out = t.hr();`
- Signature:
  `pluss::codem::Series hours(self) const`
- Explanation:
  Applies `hours` to `self`. No additional arguments.
- Example:
  `auto out = t.hours();`
- Signature:
  `pluss::codem::Series hour_(self) const`
- Explanation:
  In-place variant of `hour`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.hour_();`
- Signature:
  `pluss::codem::Series minute(self) const`
- Explanation:
  Applies `minute` to `self`. No additional arguments.
- Example:
  `auto out = t.minute();`
- Signature:
  `pluss::codem::Series m(self) const`
- Explanation:
  Applies `m` to `self`. No additional arguments.
- Example:
  `auto out = t.m();`
- Signature:
  `pluss::codem::Series minutes(self) const`
- Explanation:
  Applies `minutes` to `self`. No additional arguments.
- Example:
  `auto out = t.minutes();`
- Signature:
  `pluss::codem::Series minute_(self) const`
- Explanation:
  In-place variant of `minute`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.minute_();`
- Signature:
  `pluss::codem::Series second(self) const`
- Explanation:
  Applies `second` to `self`. No additional arguments.
- Example:
  `auto out = t.second();`
- Signature:
  `pluss::codem::Series sec(self) const`
- Explanation:
  Applies `sec` to `self`. No additional arguments.
- Example:
  `auto out = t.sec();`
- Signature:
  `pluss::codem::Series s(self) const`
- Explanation:
  Applies `s` to `self`. No additional arguments.
- Example:
  `auto out = t.s();`
- Signature:
  `pluss::codem::Series seconds(self) const`
- Explanation:
  Applies `seconds` to `self`. No additional arguments.
- Example:
  `auto out = t.seconds();`
- Signature:
  `pluss::codem::Series second_(self) const`
- Explanation:
  In-place variant of `second`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.second_();`
- Signature:
  `pluss::codem::Series nanosecond(self) const`
- Explanation:
  Applies `nanosecond` to `self`. No additional arguments.
- Example:
  `auto out = t.nanosecond();`
- Signature:
  `pluss::codem::Series ns(self) const`
- Explanation:
  Applies `ns` to `self`. No additional arguments.
- Example:
  `auto out = t.ns();`
- Signature:
  `pluss::codem::Series nsec(self) const`
- Explanation:
  Applies `nsec` to `self`. No additional arguments.
- Example:
  `auto out = t.nsec();`
- Signature:
  `pluss::codem::Series nano(self) const`
- Explanation:
  Applies `nano` to `self`. No additional arguments.
- Example:
  `auto out = t.nano();`
- Signature:
  `pluss::codem::Series nanoseconds(self) const`
- Explanation:
  Applies `nanoseconds` to `self`. No additional arguments.
- Example:
  `auto out = t.nanoseconds();`
- Signature:
  `pluss::codem::Series microsecond(self) const`
- Explanation:
  Applies `microsecond` to `self`. No additional arguments.
- Example:
  `auto out = t.microsecond();`
- Signature:
  `pluss::codem::Series us(self) const`
- Explanation:
  Applies `us` to `self`. No additional arguments.
- Example:
  `auto out = t.us();`
- Signature:
  `pluss::codem::Series usec(self) const`
- Explanation:
  Applies `usec` to `self`. No additional arguments.
- Example:
  `auto out = t.usec();`
- Signature:
  `pluss::codem::Series micro(self) const`
- Explanation:
  Applies `micro` to `self`. No additional arguments.
- Example:
  `auto out = t.micro();`
- Signature:
  `pluss::codem::Series microseconds(self) const`
- Explanation:
  Applies `microseconds` to `self`. No additional arguments.
- Example:
  `auto out = t.microseconds();`
- Signature:
  `pluss::codem::Series millisecond(self) const`
- Explanation:
  Applies `millisecond` to `self`. No additional arguments.
- Example:
  `auto out = t.millisecond();`
- Signature:
  `pluss::codem::Series ms(self) const`
- Explanation:
  Applies `ms` to `self`. No additional arguments.
- Example:
  `auto out = t.ms();`
- Signature:
  `pluss::codem::Series msec(self) const`
- Explanation:
  Applies `msec` to `self`. No additional arguments.
- Example:
  `auto out = t.msec();`
- Signature:
  `pluss::codem::Series milli(self) const`
- Explanation:
  Applies `milli` to `self`. No additional arguments.
- Example:
  `auto out = t.milli();`
- Signature:
  `pluss::codem::Series milliseconds(self) const`
- Explanation:
  Applies `milliseconds` to `self`. No additional arguments.
- Example:
  `auto out = t.milliseconds();`
- Signature:
  `pluss::codem::Series weekday(self) const`
- Explanation:
  Applies `weekday` to `self`. No additional arguments.
- Example:
  `auto out = t.weekday();`
- Signature:
  `pluss::codem::Series dayofweek(self) const`
- Explanation:
  Applies `dayofweek` to `self`. No additional arguments.
- Example:
  `auto out = t.dayofweek();`
- Signature:
  `pluss::codem::Series dow(self) const`
- Explanation:
  Applies `dow` to `self`. No additional arguments.
- Example:
  `auto out = t.dow();`
- Signature:
  `pluss::codem::Series weekday_(self) const`
- Explanation:
  In-place variant of `weekday`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.weekday_();`
- Signature:
  `pluss::codem::Series dayofyear(self) const`
- Explanation:
  Applies `dayofyear` to `self`. No additional arguments.
- Example:
  `auto out = t.dayofyear();`
- Signature:
  `pluss::codem::Series doy(self) const`
- Explanation:
  Applies `doy` to `self`. No additional arguments.
- Example:
  `auto out = t.doy();`
- Signature:
  `pluss::codem::Series day_of_year(self) const`
- Explanation:
  Applies `day_of_year` to `self`. No additional arguments.
- Example:
  `auto out = t.day_of_year();`
- Signature:
  `pluss::codem::Series dayofyear_(self) const`
- Explanation:
  In-place variant of `dayofyear`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.dayofyear_();`
- Signature:
  `pluss::codem::Series quarter(self) const`
- Explanation:
  Applies `quarter` to `self`. No additional arguments.
- Example:
  `auto out = t.quarter();`
- Signature:
  `pluss::codem::Series q(self) const`
- Explanation:
  Applies `q` to `self`. No additional arguments.
- Example:
  `auto out = t.q();`
- Signature:
  `pluss::codem::Series qtr(self) const`
- Explanation:
  Applies `qtr` to `self`. No additional arguments.
- Example:
  `auto out = t.qtr();`
- Signature:
  `pluss::codem::Series quarter_(self) const`
- Explanation:
  In-place variant of `quarter`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.quarter_();`
- Signature:
  `pluss::codem::Series week(self) const`
- Explanation:
  Applies `week` to `self`. No additional arguments.
- Example:
  `auto out = t.week();`
- Signature:
  `pluss::codem::Series weekofyear(self) const`
- Explanation:
  Applies `weekofyear` to `self`. No additional arguments.
- Example:
  `auto out = t.weekofyear();`
- Signature:
  `pluss::codem::Series woy(self) const`
- Explanation:
  Applies `woy` to `self`. No additional arguments.
- Example:
  `auto out = t.woy();`
- Signature:
  `pluss::codem::Series week_(self) const`
- Explanation:
  In-place variant of `week`; it mutates `self` and returns `self`. No additional arguments.
- Example:
  `auto out = t.week_();`
- Signature:
  `pluss::codem::Series is_leap_year(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_leap_year();`
- Signature:
  `pluss::codem::Series leap_year(self) const`
- Explanation:
  Applies `leap_year` to `self`. No additional arguments.
- Example:
  `auto out = t.leap_year();`
- Signature:
  `pluss::codem::Series isleapyear(self) const`
- Explanation:
  Applies `isleapyear` to `self`. No additional arguments.
- Example:
  `auto out = t.isleapyear();`
- Signature:
  `pluss::codem::Series is_leap(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_leap();`
- Signature:
  `pluss::codem::Series is_month_start(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_start();`
- Signature:
  `pluss::codem::Series is_month_begin(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_begin();`
- Signature:
  `pluss::codem::Series is_monthstart(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_monthstart();`
- Signature:
  `pluss::codem::Series is_month_end(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_end();`
- Signature:
  `pluss::codem::Series is_month_finish(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_month_finish();`
- Signature:
  `pluss::codem::Series is_monthend(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_monthend();`
- Signature:
  `pluss::codem::Series is_quarter_start(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_start();`
- Signature:
  `pluss::codem::Series is_qs(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_qs();`
- Signature:
  `pluss::codem::Series is_quarter_begin(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_begin();`
- Signature:
  `pluss::codem::Series is_quarterstart(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarterstart();`
- Signature:
  `pluss::codem::Series is_quarter_end(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_end();`
- Signature:
  `pluss::codem::Series is_qe(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_qe();`
- Signature:
  `pluss::codem::Series is_quarter_finish(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarter_finish();`
- Signature:
  `pluss::codem::Series is_quarterend(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_quarterend();`
- Signature:
  `pluss::codem::Series is_year_start(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_year_start();`
- Signature:
  `pluss::codem::Series is_year_begin(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_year_begin();`
- Signature:
  `pluss::codem::Series is_yearstart(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_yearstart();`
- Signature:
  `pluss::codem::Series is_year_end(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_year_end();`
- Signature:
  `pluss::codem::Series is_year_finish(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_year_finish();`
- Signature:
  `pluss::codem::Series is_yearend(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_yearend();`
- Signature:
  `pluss::codem::Series is_weekend(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_weekend();`
- Signature:
  `pluss::codem::Series weekend(self) const`
- Explanation:
  Applies `weekend` to `self`. No additional arguments.
- Example:
  `auto out = t.weekend();`
- Signature:
  `pluss::codem::Series is_sat_or_sun(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_sat_or_sun();`
- Signature:
  `pluss::codem::Series is_business_day(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_business_day();`
- Signature:
  `pluss::codem::Series is_bday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_bday();`
- Signature:
  `pluss::codem::Series is_businessday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_businessday();`
- Signature:
  `pluss::codem::Series business_day(self) const`
- Explanation:
  Applies `business_day` to `self`. No additional arguments.
- Example:
  `auto out = t.business_day();`
- Signature:
  `pluss::codem::Series is_monday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_monday();`
- Signature:
  `pluss::codem::Series mon_is(self) const`
- Explanation:
  Applies `mon_is` to `self`. No additional arguments.
- Example:
  `auto out = t.mon_is();`
- Signature:
  `pluss::codem::Series is_mon(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_mon();`
- Signature:
  `pluss::codem::Series is_tuesday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_tuesday();`
- Signature:
  `pluss::codem::Series tue_is(self) const`
- Explanation:
  Applies `tue_is` to `self`. No additional arguments.
- Example:
  `auto out = t.tue_is();`
- Signature:
  `pluss::codem::Series is_tue(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_tue();`
- Signature:
  `pluss::codem::Series is_wednesday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_wednesday();`
- Signature:
  `pluss::codem::Series wed_is(self) const`
- Explanation:
  Applies `wed_is` to `self`. No additional arguments.
- Example:
  `auto out = t.wed_is();`
- Signature:
  `pluss::codem::Series is_wed(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_wed();`
- Signature:
  `pluss::codem::Series is_thursday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_thursday();`
- Signature:
  `pluss::codem::Series thu_is(self) const`
- Explanation:
  Applies `thu_is` to `self`. No additional arguments.
- Example:
  `auto out = t.thu_is();`
- Signature:
  `pluss::codem::Series is_thu(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_thu();`
- Signature:
  `pluss::codem::Series is_friday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_friday();`
- Signature:
  `pluss::codem::Series fri_is(self) const`
- Explanation:
  Applies `fri_is` to `self`. No additional arguments.
- Example:
  `auto out = t.fri_is();`
- Signature:
  `pluss::codem::Series is_fri(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_fri();`
- Signature:
  `pluss::codem::Series is_saturday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_saturday();`
- Signature:
  `pluss::codem::Series sat_is(self) const`
- Explanation:
  Applies `sat_is` to `self`. No additional arguments.
- Example:
  `auto out = t.sat_is();`
- Signature:
  `pluss::codem::Series is_sat(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_sat();`
- Signature:
  `pluss::codem::Series is_sunday(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_sunday();`
- Signature:
  `pluss::codem::Series sun_is(self) const`
- Explanation:
  Applies `sun_is` to `self`. No additional arguments.
- Example:
  `auto out = t.sun_is();`
- Signature:
  `pluss::codem::Series is_sun(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = t.is_sun();`
- Signature:
  `pluss::coden::Note to_note_date(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date();`
- Signature:
  `pluss::coden::Note to_note_date_compact(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_compact();`
- Signature:
  `pluss::coden::Note to_note_time(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time();`
- Signature:
  `pluss::coden::Note to_note_time_hm(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_hm();`
- Signature:
  `pluss::coden::Note to_note_datetime(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_datetime();`
- Signature:
  `pluss::coden::Note to_note_rfc3339(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339();`
- Signature:
  `pluss::coden::Note to_note_rfc3339_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339_nano();`
- Signature:
  `pluss::coden::Note to_note_ym(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_ym();`
- Signature:
  `pluss::coden::Note to_note_my(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_my();`
- Signature:
  `pluss::coden::Note to_note_md(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_md();`
- Signature:
  `pluss::coden::Note to_note_dmy(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmy();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_slash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_slash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_slash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_slash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_slash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_slash();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_slash(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_slash();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dot();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dot();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dot();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dot();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dot();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dot();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dot(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dot();`
- Signature:
  `pluss::coden::Note to_note_date_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_z();`
- Signature:
  `pluss::coden::Note to_note_date_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_nano();`
- Signature:
  `pluss::coden::Note to_note_date_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_nano_z();`
- Signature:
  `pluss::coden::Note to_note_date_compact_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_compact_z();`
- Signature:
  `pluss::coden::Note to_note_date_compact_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_compact_nano();`
- Signature:
  `pluss::coden::Note to_note_date_compact_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_date_compact_nano_z();`
- Signature:
  `pluss::coden::Note to_note_time_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_z();`
- Signature:
  `pluss::coden::Note to_note_time_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_nano();`
- Signature:
  `pluss::coden::Note to_note_time_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_nano_z();`
- Signature:
  `pluss::coden::Note to_note_time_hm_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_hm_z();`
- Signature:
  `pluss::coden::Note to_note_time_hm_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_hm_nano();`
- Signature:
  `pluss::coden::Note to_note_time_hm_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_time_hm_nano_z();`
- Signature:
  `pluss::coden::Note to_note_datetime_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_datetime_z();`
- Signature:
  `pluss::coden::Note to_note_datetime_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_datetime_nano();`
- Signature:
  `pluss::coden::Note to_note_datetime_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_datetime_nano_z();`
- Signature:
  `pluss::coden::Note to_note_rfc3339_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339_z();`
- Signature:
  `pluss::coden::Note to_note_rfc3339_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339_nano_z();`
- Signature:
  `pluss::coden::Note to_note_rfc3339_nano_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339_nano_nano();`
- Signature:
  `pluss::coden::Note to_note_rfc3339_nano_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_rfc3339_nano_nano_z();`
- Signature:
  `pluss::coden::Note to_note_ym_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_ym_z();`
- Signature:
  `pluss::coden::Note to_note_ym_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_ym_nano();`
- Signature:
  `pluss::coden::Note to_note_ym_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_ym_nano_z();`
- Signature:
  `pluss::coden::Note to_note_my_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_my_z();`
- Signature:
  `pluss::coden::Note to_note_my_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_my_nano();`
- Signature:
  `pluss::coden::Note to_note_my_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_my_nano_z();`
- Signature:
  `pluss::coden::Note to_note_md_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_md_z();`
- Signature:
  `pluss::coden::Note to_note_md_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_md_nano();`
- Signature:
  `pluss::coden::Note to_note_md_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_md_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmy_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmy_z();`
- Signature:
  `pluss::coden::Note to_note_dmy_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmy_nano();`
- Signature:
  `pluss::coden::Note to_note_dmy_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmy_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_slash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_slash_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_slash_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_slash_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_slash_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_slash_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_slash_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hm_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hm_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_Ymd_hms_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_Ymd_hms_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hm_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hm_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_dmY_hms_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_dmY_hms_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hm_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hm_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dot_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_sp_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_sp_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dot_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dot_z();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dot_nano(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dot_nano();`
- Signature:
  `pluss::coden::Note to_note_mdY_hms_t_dot_nano_z(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_mdY_hms_t_dot_nano_z();`
- Signature:
  `pluss::coden::Note to_note_weekday(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_weekday();`
- Signature:
  `pluss::coden::Note to_note_weekday_long(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_weekday_long();`
- Signature:
  `pluss::coden::Note to_note_month_name(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_month_name();`
- Signature:
  `pluss::coden::Note to_note_month_name_long(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_month_name_long();`
- Signature:
  `pluss::coden::Note to_note_month_year_name(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_month_year_name();`
- Signature:
  `pluss::coden::Note to_note_iso_week(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_iso_week();`
- Signature:
  `pluss::coden::Note to_note_iso_week_compact(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = t.to_note_iso_week_compact();`
- Signature:
  `Timeline::createFromDateTimeInts(int year, int month, int day, int hour, int minute, int second)`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `int year, int month, int day, int hour, int minute, int second`.
- Example:
  `auto out = pluss::codeh::Timeline::createFromDateTimeInts(2024, 1, 10, 8, 30, 0);`
- Signature:
  `Timeline::date_range(const Timeline& start, const Timeline& end, std::string_view freq = "D", int step = 1)`
- Explanation:
  Static helper that creates a regular timeline range between start and end. Arguments: `const Timeline& start, const Timeline& end, std::string_view freq = "D", int step = 1`.
- Example:
  `auto out = pluss::codeh::Timeline::date_range(t, other_t, "D", 1);`
- Signature:
  `Timeline::from_ymd(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, std::string_view tz_name = "UTC")`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, std::string_view tz_name = "UTC"`.
- Example:
  `auto out = pluss::codeh::Timeline::from_ymd(year_col, month_col, day_col, "UTC");`
- Signature:
  `Timeline::from_ymdhms(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, const pluss::codem::Series& hour, const pluss::codem::Series& minute, const pluss::codem::Series& second, std::string_view tz_name = "UTC")`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, const pluss::codem::Series& hour, const pluss::codem::Series& minute, const pluss::codem::Series& second, std::string_view tz_name = "UTC"`.
- Example:
  `auto out = pluss::codeh::Timeline::from_ymdhms(year_col, month_col, day_col, hour_col, minute_col, second_col, "UTC");`
- Signature:
  `Timeline::from_epoch_seconds(const pluss::codem::Series& epoch_seconds, std::string_view tz_name = "UTC")`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_seconds, std::string_view tz_name = "UTC"`.
- Example:
  `auto out = pluss::codeh::Timeline::from_epoch_seconds(series_vals, "UTC");`
- Signature:
  `Timeline::from_epoch_millis(const pluss::codem::Series& epoch_millis, std::string_view tz_name = "UTC")`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_millis, std::string_view tz_name = "UTC"`.
- Example:
  `auto out = pluss::codeh::Timeline::from_epoch_millis(series_vals, "UTC");`
- Signature:
  `Timeline::from_epoch_micros(const pluss::codem::Series& epoch_micros, std::string_view tz_name = "UTC")`
- Explanation:
  Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_micros, std::string_view tz_name = "UTC"`.
- Example:
  `auto out = pluss::codeh::Timeline::from_epoch_micros(series_vals, "UTC");`

## Implemented TimeIndex API: Signatures + Real Calls

Only methods with concrete runtime routes in `CodeH/compat/Dispatch.cpp` are documented here.
Signature convention: `self` is the receiver object for instance methods.

# Complete TimeIndex Method Reference: Signatures + Real Calls

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});
auto key_t = pluss::moment("2024-01-01T00:20:00Z");
auto values = pluss::value(std::vector<double>{10.0, 20.0, 30.0}, {3});
auto indices = pluss::make_series(std::vector<std::int64_t>{0,2}, {2});
auto dur = pluss::duration_minutes(30);
auto tol = pluss::duration_minutes(20);
```

- Signature:
  `std::int64_t searchsorted(self, const Timeline& key, bool right) const`
- Explanation:
  Applies `searchsorted` to `self`. Arguments: `const Timeline& key, bool right`.
- Example:
  `auto out = idx.searchsorted(key_t, true);`
- Signature:
  `pluss::codem::Series slice(self, const Timeline& start, const Timeline& end) const`
- Explanation:
  Applies `slice` to `self`. Arguments: `const Timeline& start, const Timeline& end`.
- Example:
  `auto out = idx.slice(key_t, key_t);`
- Signature:
  `pluss::codem::Series slice_time(self, std::string_view start_iso, std::string_view end_iso) const`
- Explanation:
  Applies `slice_time` to `self`. Arguments: `std::string_view start_iso, std::string_view end_iso`.
- Example:
  `auto out = idx.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");`
- Signature:
  `std::int64_t asof(self, const Timeline& key) const`
- Explanation:
  Applies `asof` to `self`. Arguments: `const Timeline& key`.
- Example:
  `auto out = idx.asof(key_t);`
- Signature:
  `ResampleBins resample_bins(self, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_bins("H", 1);`
- Signature:
  `pluss::codem::Series resample_count(self, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_count("H", 1);`
- Signature:
  `pluss::codem::Series resample_sum(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_sum(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_mean(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_mean(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_min(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_min(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_max(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_max(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_first(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_first(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_last(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_last(values, "H", 1);`
- Signature:
  `pluss::codem::Series resample_ohlc(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_ohlc(values, "H", 1);`
- Signature:
  `pluss::codem::Series rolling_count(self, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const Duration& window`.
- Example:
  `auto out = idx.rolling_count(dur);`
- Signature:
  `pluss::codem::Series rolling_sum(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_sum(values, dur);`
- Signature:
  `AlignUnion align_union(self, const TimeIndex& other) const`
- Explanation:
  Applies `align_union` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.align_union(other_idx);`
- Signature:
  `bool is_monotonic_increasing(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = idx.is_monotonic_increasing();`
- Signature:
  `bool is_monotonic_decreasing(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = idx.is_monotonic_decreasing();`
- Signature:
  `bool is_unique(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = idx.is_unique();`
- Signature:
  `bool has_duplicates(self) const`
- Explanation:
  Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.
- Example:
  `auto out = idx.has_duplicates();`
- Signature:
  `Timeline min(self) const`
- Explanation:
  Applies `min` to `self`. No additional arguments.
- Example:
  `auto out = idx.min();`
- Signature:
  `Timeline max(self) const`
- Explanation:
  Applies `max` to `self`. No additional arguments.
- Example:
  `auto out = idx.max();`
- Signature:
  `TimeIndex sort(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = idx.sort();`
- Signature:
  `TimeIndex unique(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = idx.unique();`
- Signature:
  `TimeIndex drop_duplicates(self) const`
- Explanation:
  Applies `drop_duplicates` to `self`. No additional arguments.
- Example:
  `auto out = idx.drop_duplicates();`
- Signature:
  `pluss::codem::Series argsort(self) const`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = idx.argsort();`
- Signature:
  `TimeIndex take(self, const pluss::codem::Series& indices) const`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const pluss::codem::Series& indices`.
- Example:
  `auto out = idx.take(indices);`
- Signature:
  `TimeIndex slice_index(self, std::int64_t start, std::int64_t stop) const`
- Explanation:
  Applies `slice_index` to `self`. Arguments: `std::int64_t start, std::int64_t stop`.
- Example:
  `auto out = idx.slice_index(1, 1);`
- Signature:
  `TimeIndex head(self, std::int64_t n) const`
- Explanation:
  Applies `head` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto out = idx.head(1);`
- Signature:
  `TimeIndex tail(self, std::int64_t n) const`
- Explanation:
  Applies `tail` to `self`. Arguments: `std::int64_t n`.
- Example:
  `auto out = idx.tail(1);`
- Signature:
  `TimeIndex union_(self, const TimeIndex& other) const`
- Explanation:
  In-place variant of `union`; it mutates `self` and returns `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.union_(other_idx);`
- Signature:
  `TimeIndex intersection(self, const TimeIndex& other) const`
- Explanation:
  Applies `intersection` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.intersection(other_idx);`
- Signature:
  `TimeIndex difference(self, const TimeIndex& other) const`
- Explanation:
  Applies `difference` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.difference(other_idx);`
- Signature:
  `TimeIndex tz_convert(self, std::string_view tz_name) const`
- Explanation:
  Applies `tz_convert` to `self`. Arguments: `std::string_view tz_name`.
- Example:
  `auto out = idx.tz_convert("UTC");`
- Signature:
  `TimeIndex tz_localize(self, std::string_view tz_name, int fold, std::string_view nonexistent) const`
- Explanation:
  Applies `tz_localize` to `self`. Arguments: `std::string_view tz_name, int fold, std::string_view nonexistent`.
- Example:
  `auto out = idx.tz_localize("UTC", 1, "raise");`
- Signature:
  `TimeIndex to_utc(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = idx.to_utc();`
- Signature:
  `TimeIndex floor(self, std::string_view unit) const`
- Explanation:
  Applies `floor` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = idx.floor("D");`
- Signature:
  `TimeIndex ceil(self, std::string_view unit) const`
- Explanation:
  Applies `ceil` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = idx.ceil("D");`
- Signature:
  `TimeIndex round(self, std::string_view unit) const`
- Explanation:
  Applies `round` computation to `self`. Arguments: `std::string_view unit`.
- Example:
  `auto out = idx.round("D");`
- Signature:
  `TimeIndex normalize(self) const`
- Explanation:
  Applies `normalize` to `self`. No additional arguments.
- Example:
  `auto out = idx.normalize();`
- Signature:
  `pluss::codem::Series resample_agg(self, const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step`.
- Example:
  `auto out = idx.resample_agg(values, "H", "sum", 1);`
- Signature:
  `pluss::codem::Series resample_size(self, std::string_view freq, std::int64_t step) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.
- Example:
  `auto out = idx.resample_size("H", 1);`
- Signature:
  `pluss::codem::Series rolling_agg(self, const pluss::codem::Series& values, const Duration& window, std::string_view op) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window, std::string_view op`.
- Example:
  `auto out = idx.rolling_agg(values, dur, "sum");`
- Signature:
  `pluss::codem::Series rolling_mean(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_mean(values, dur);`
- Signature:
  `pluss::codem::Series rolling_min(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_min(values, dur);`
- Signature:
  `pluss::codem::Series rolling_max(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_max(values, dur);`
- Signature:
  `pluss::codem::Series rolling_first(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_first(values, dur);`
- Signature:
  `pluss::codem::Series rolling_last(self, const pluss::codem::Series& values, const Duration& window) const`
- Explanation:
  Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.
- Example:
  `auto out = idx.rolling_last(values, dur);`
- Signature:
  `std::string infer_freq(self) const`
- Explanation:
  Applies `infer_freq` to `self`. No additional arguments.
- Example:
  `auto out = idx.infer_freq();`
- Signature:
  `std::string freq(self) const`
- Explanation:
  Applies `freq` to `self`. No additional arguments.
- Example:
  `auto out = idx.freq();`
- Signature:
  `std::string frequency(self) const`
- Explanation:
  Applies `frequency` to `self`. No additional arguments.
- Example:
  `auto out = idx.frequency();`
- Signature:
  `pluss::coden::Note to_note_iso(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = idx.to_note_iso();`
- Signature:
  `pluss::coden::Note tostring(self) const`
- Explanation:
  Applies `tostring` to `self`. No additional arguments.
- Example:
  `auto out = idx.tostring();`
- Signature:
  `pluss::coden::Note to_string(self) const`
- Explanation:
  Converts between related PlussQit data types. No additional arguments.
- Example:
  `auto out = idx.to_string();`
- Signature:
  `pluss::coden::Note format_iso(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = idx.format_iso();`
- Signature:
  `pluss::coden::Note format(self) const`
- Explanation:
  Formats/converts `self` to a textual representation. No additional arguments.
- Example:
  `auto out = idx.format();`
- Signature:
  `TimeIndex sort_values(self) const`
- Explanation:
  Applies `sort_values` to `self`. No additional arguments.
- Example:
  `auto out = idx.sort_values();`
- Signature:
  `TimeIndex sort_index(self) const`
- Explanation:
  Applies `sort_index` to `self`. No additional arguments.
- Example:
  `auto out = idx.sort_index();`
- Signature:
  `TimeIndex unionIndex(self, const TimeIndex& other) const`
- Explanation:
  Applies `unionIndex` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.unionIndex(other_idx);`
- Signature:
  `TimeIndex intersect(self, const TimeIndex& other) const`
- Explanation:
  Applies `intersect` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.intersect(other_idx);`
- Signature:
  `TimeIndex diffIndex(self, const TimeIndex& other) const`
- Explanation:
  Applies `diffIndex` to `self`. Arguments: `const TimeIndex& other`.
- Example:
  `auto out = idx.diffIndex(other_idx);`
## Demos to run

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`
