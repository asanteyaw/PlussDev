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


## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

### `add_days`

#### Signature

```cpp
Timeline add_days(self, int days) const
```

#### Parameters

`days` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_days` to `self`. Arguments: `int days`.

#### Examples

```cpp
auto out = t.add_days(10);
```

### `add_duration`

#### Signature

```cpp
Timeline add_duration(self, const Duration& d) const
```

#### Parameters

`d` : `const Duration&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_duration` to `self`. Arguments: `const Duration& d`.

#### Examples

```cpp
auto out = t.add_duration(dur);
```

### `add_hours`

#### Signature

```cpp
Timeline add_hours(self, int hours) const
```

#### Parameters

`hours` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_hours` to `self`. Arguments: `int hours`.

#### Examples

```cpp
auto out = t.add_hours(6);
```

### `add_minutes`

#### Signature

```cpp
Timeline add_minutes(self, int minutes) const
```

#### Parameters

`minutes` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_minutes` to `self`. Arguments: `int minutes`.

#### Examples

```cpp
auto out = t.add_minutes(45);
```

### `add_months`

#### Signature

```cpp
Timeline add_months(self, int months) const
```

#### Parameters

`months` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_months` to `self`. Arguments: `int months`.

#### Examples

```cpp
auto out = t.add_months(3);
```

### `add_nanos`

#### Signature

```cpp
Timeline add_nanos(self, std::int64_t nanos) const
```

#### Parameters

`nanos` : `std::int64_t`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_nanos` to `self`. Arguments: `std::int64_t nanos`.

#### Examples

```cpp
auto out = t.add_nanos(1'000'000);
```

### `add_seconds`

#### Signature

```cpp
Timeline add_seconds(self, int seconds) const
```

#### Parameters

`seconds` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_seconds` to `self`. Arguments: `int seconds`.

#### Examples

```cpp
auto out = t.add_seconds(30);
```

### `add_years`

#### Signature

```cpp
Timeline add_years(self, int years) const
```

#### Parameters

`years` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `add_years` to `self`. Arguments: `int years`.

#### Examples

```cpp
auto out = t.add_years(2);
```

### `argsort`

#### Signature

```cpp
pluss::codem::Series argsort(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = t.argsort();
```

### `business_day`

#### Signature

```cpp
pluss::codem::Series business_day(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `business_day` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.business_day();
```

### `ceil`

#### Signature

```cpp
Timeline ceil(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `ceil` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = t.ceil("D");
```

### `concat`

#### Signature

```cpp
Timeline concat(self, const std::vector<Timeline>& others) const
```

#### Parameters

`others` : `const std::vector<Timeline>&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<Timeline>& others`.

#### See also

- `Timeline concat(self, const Timeline& other) const`

#### Examples

```cpp
auto out = t.concat(others);
```

### `concat`

#### Signature

```cpp
Timeline concat(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const Timeline& other`.

#### See also

- `Timeline concat(self, const std::vector<Timeline>& others) const`

#### Examples

```cpp
auto out = t.concat(other_t);
```

### `d`

#### Signature

```cpp
pluss::codem::Series d(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `d` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.d();
```

### `date`

#### Signature

```cpp
Timeline date(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `date` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.date();
```

### `day`

#### Signature

```cpp
pluss::codem::Series day(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `day` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.day();
```

### `day_`

#### Signature

```cpp
pluss::codem::Series day_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `day`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.day_();
```

### `day_of_year`

#### Signature

```cpp
pluss::codem::Series day_of_year(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `day_of_year` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.day_of_year();
```

### `day_of_year_scalar`

#### Signature

```cpp
int day_of_year_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `day_of_year_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.day_of_year_scalar();
```

### `day_scalar`

#### Signature

```cpp
int day_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `day_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.day_scalar();
```

### `dayofweek`

#### Signature

```cpp
pluss::codem::Series dayofweek(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `dayofweek` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.dayofweek();
```

### `dayofyear`

#### Signature

```cpp
pluss::codem::Series dayofyear(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `dayofyear` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.dayofyear();
```

### `dayofyear_`

#### Signature

```cpp
pluss::codem::Series dayofyear_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `dayofyear`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.dayofyear_();
```

### `days`

#### Signature

```cpp
pluss::codem::Series days(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `days` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.days();
```

### `days_between`

#### Signature

```cpp
int days_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `days_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.days_between(other_t);
```

### `diff`

#### Signature

```cpp
Duration diff(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`Duration`
    Return value produced by this method call.

#### Notes

Applies `diff` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.diff(other_t);
```

### `dom`

#### Signature

```cpp
pluss::codem::Series dom(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `dom` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.dom();
```

### `dow`

#### Signature

```cpp
pluss::codem::Series dow(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `dow` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.dow();
```

### `doy`

#### Signature

```cpp
pluss::codem::Series doy(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `doy` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.doy();
```

### `drop_duplicates`

#### Signature

```cpp
Timeline drop_duplicates(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `drop_duplicates` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.drop_duplicates();
```

### `empty_like`

#### Signature

```cpp
Timeline empty_like(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `empty_like` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.empty_like();
```

### `epoch_micros`

#### Signature

```cpp
pluss::codem::Series epoch_micros(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `epoch_micros` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.epoch_micros();
```

### `epoch_millis`

#### Signature

```cpp
pluss::codem::Series epoch_millis(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `epoch_millis` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.epoch_millis();
```

### `epoch_nanos`

#### Signature

```cpp
pluss::codem::Series epoch_nanos(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `epoch_nanos` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.epoch_nanos();
```

### `epoch_seconds`

#### Signature

```cpp
pluss::codem::Series epoch_seconds(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `epoch_seconds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.epoch_seconds();
```

### `eq`

#### Signature

```cpp
pluss::codem::Series eq(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `eq` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.eq(other_t);
```

### `fill_like`

#### Signature

```cpp
Timeline fill_like(self, const std::string& date_str) const
```

#### Parameters

`date_str` : `const std::string&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `fill_like` to `self`. Arguments: `const std::string& date_str`.

#### See also

- `Timeline fill_like(self, int year, int month, int day) const`

#### Examples

```cpp
auto out = t.fill_like("2024-12-25T00:00:00Z");
```

### `fill_like`

#### Signature

```cpp
Timeline fill_like(self, int year, int month, int day) const
```

#### Parameters

`year` : `int`
    Required method argument.

`month` : `int`
    Required method argument.

`day` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `fill_like` to `self`. Arguments: `int year, int month, int day`.

#### See also

- `Timeline fill_like(self, const std::string& date_str) const`

#### Examples

```cpp
auto out = t.fill_like(2024, 12, 25);
```

### `fill_nat`

#### Signature

```cpp
Timeline fill_nat(self, const Timeline& value) const
```

#### Parameters

`value` : `const Timeline&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `fill_nat` to `self`. Arguments: `const Timeline& value`.

#### Examples

```cpp
auto out = t.fill_nat(other_t);
```

### `floor`

#### Signature

```cpp
Timeline floor(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `floor` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = t.floor("D");
```

### `format`

#### Signature

```cpp
std::string format(self, const std::string& format_str) const
```

#### Parameters

`format_str` : `const std::string&`
    Required method argument.

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. Arguments: `const std::string& format_str`.

#### Examples

```cpp
auto out = t.format("%Y-%m-%d");
```

### `fri_is`

#### Signature

```cpp
pluss::codem::Series fri_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `fri_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.fri_is();
```

### `ge`

#### Signature

```cpp
pluss::codem::Series ge(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `ge` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.ge(other_t);
```

### `gt`

#### Signature

```cpp
pluss::codem::Series gt(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `gt` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.gt(other_t);
```

### `h`

#### Signature

```cpp
pluss::codem::Series h(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `h` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.h();
```

### `hour`

#### Signature

```cpp
pluss::codem::Series hour(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `hour` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.hour();
```

### `hour_`

#### Signature

```cpp
pluss::codem::Series hour_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `hour`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.hour_();
```

### `hour_scalar`

#### Signature

```cpp
int hour_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `hour_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.hour_scalar();
```

### `hours`

#### Signature

```cpp
pluss::codem::Series hours(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `hours` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.hours();
```

### `hours_between`

#### Signature

```cpp
int hours_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `hours_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.hours_between(other_t);
```

### `hr`

#### Signature

```cpp
pluss::codem::Series hr(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `hr` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.hr();
```

### `is_bday`

#### Signature

```cpp
pluss::codem::Series is_bday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_bday();
```

### `is_business_day`

#### Signature

```cpp
pluss::codem::Series is_business_day(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_business_day();
```

### `is_businessday`

#### Signature

```cpp
pluss::codem::Series is_businessday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_businessday();
```

### `is_fri`

#### Signature

```cpp
pluss::codem::Series is_fri(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_fri();
```

### `is_friday`

#### Signature

```cpp
pluss::codem::Series is_friday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_friday();
```

### `is_leap`

#### Signature

```cpp
pluss::codem::Series is_leap(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_leap();
```

### `is_leap_year`

#### Signature

```cpp
pluss::codem::Series is_leap_year(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_leap_year();
```

### `is_leap_year_scalar`

#### Signature

```cpp
bool is_leap_year_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_leap_year_scalar();
```

### `is_mon`

#### Signature

```cpp
pluss::codem::Series is_mon(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_mon();
```

### `is_monday`

#### Signature

```cpp
pluss::codem::Series is_monday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_monday();
```

### `is_month_begin`

#### Signature

```cpp
pluss::codem::Series is_month_begin(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_begin();
```

### `is_month_end`

#### Signature

```cpp
pluss::codem::Series is_month_end(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_end();
```

### `is_month_end_scalar`

#### Signature

```cpp
bool is_month_end_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_end_scalar();
```

### `is_month_finish`

#### Signature

```cpp
pluss::codem::Series is_month_finish(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_finish();
```

### `is_month_start`

#### Signature

```cpp
pluss::codem::Series is_month_start(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_start();
```

### `is_month_start_scalar`

#### Signature

```cpp
bool is_month_start_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_month_start_scalar();
```

### `is_monthend`

#### Signature

```cpp
pluss::codem::Series is_monthend(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_monthend();
```

### `is_monthstart`

#### Signature

```cpp
pluss::codem::Series is_monthstart(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_monthstart();
```

### `is_qe`

#### Signature

```cpp
pluss::codem::Series is_qe(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_qe();
```

### `is_qs`

#### Signature

```cpp
pluss::codem::Series is_qs(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_qs();
```

### `is_quarter_begin`

#### Signature

```cpp
pluss::codem::Series is_quarter_begin(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_begin();
```

### `is_quarter_end`

#### Signature

```cpp
pluss::codem::Series is_quarter_end(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_end();
```

### `is_quarter_end_scalar`

#### Signature

```cpp
bool is_quarter_end_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_end_scalar();
```

### `is_quarter_finish`

#### Signature

```cpp
pluss::codem::Series is_quarter_finish(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_finish();
```

### `is_quarter_start`

#### Signature

```cpp
pluss::codem::Series is_quarter_start(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_start();
```

### `is_quarter_start_scalar`

#### Signature

```cpp
bool is_quarter_start_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarter_start_scalar();
```

### `is_quarterend`

#### Signature

```cpp
pluss::codem::Series is_quarterend(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarterend();
```

### `is_quarterstart`

#### Signature

```cpp
pluss::codem::Series is_quarterstart(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_quarterstart();
```

### `is_sat`

#### Signature

```cpp
pluss::codem::Series is_sat(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_sat();
```

### `is_sat_or_sun`

#### Signature

```cpp
pluss::codem::Series is_sat_or_sun(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_sat_or_sun();
```

### `is_saturday`

#### Signature

```cpp
pluss::codem::Series is_saturday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_saturday();
```

### `is_sun`

#### Signature

```cpp
pluss::codem::Series is_sun(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_sun();
```

### `is_sunday`

#### Signature

```cpp
pluss::codem::Series is_sunday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_sunday();
```

### `is_thu`

#### Signature

```cpp
pluss::codem::Series is_thu(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_thu();
```

### `is_thursday`

#### Signature

```cpp
pluss::codem::Series is_thursday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_thursday();
```

### `is_tue`

#### Signature

```cpp
pluss::codem::Series is_tue(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_tue();
```

### `is_tuesday`

#### Signature

```cpp
pluss::codem::Series is_tuesday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_tuesday();
```

### `is_wed`

#### Signature

```cpp
pluss::codem::Series is_wed(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_wed();
```

### `is_wednesday`

#### Signature

```cpp
pluss::codem::Series is_wednesday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_wednesday();
```

### `is_weekend`

#### Signature

```cpp
pluss::codem::Series is_weekend(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_weekend();
```

### `is_weekend_scalar`

#### Signature

```cpp
bool is_weekend_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_weekend_scalar();
```

### `is_year_begin`

#### Signature

```cpp
pluss::codem::Series is_year_begin(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_year_begin();
```

### `is_year_end`

#### Signature

```cpp
pluss::codem::Series is_year_end(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_year_end();
```

### `is_year_finish`

#### Signature

```cpp
pluss::codem::Series is_year_finish(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_year_finish();
```

### `is_year_start`

#### Signature

```cpp
pluss::codem::Series is_year_start(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_year_start();
```

### `is_yearend`

#### Signature

```cpp
pluss::codem::Series is_yearend(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_yearend();
```

### `is_yearstart`

#### Signature

```cpp
pluss::codem::Series is_yearstart(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = t.is_yearstart();
```

### `isleapyear`

#### Signature

```cpp
pluss::codem::Series isleapyear(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `isleapyear` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.isleapyear();
```

### `isnat`

#### Signature

```cpp
pluss::codem::Series isnat(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `isnat` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.isnat();
```

### `isnat_scalar`

#### Signature

```cpp
bool isnat_scalar(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Applies `isnat_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.isnat_scalar();
```

### `isoformat`

#### Signature

```cpp
std::string isoformat(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `isoformat` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.isoformat();
```

### `isvalid`

#### Signature

```cpp
pluss::codem::Series isvalid(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `isvalid` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.isvalid();
```

### `le`

#### Signature

```cpp
pluss::codem::Series le(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `le` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.le(other_t);
```

### `leap_year`

#### Signature

```cpp
pluss::codem::Series leap_year(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `leap_year` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.leap_year();
```

### `lt`

#### Signature

```cpp
pluss::codem::Series lt(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `lt` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.lt(other_t);
```

### `m`

#### Signature

```cpp
pluss::codem::Series m(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `m` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.m();
```

### `max`

#### Signature

```cpp
Timeline max(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `max` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.max();
```

### `micro`

#### Signature

```cpp
pluss::codem::Series micro(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `micro` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.micro();
```

### `microsecond`

#### Signature

```cpp
pluss::codem::Series microsecond(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `microsecond` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.microsecond();
```

### `microseconds`

#### Signature

```cpp
pluss::codem::Series microseconds(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `microseconds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.microseconds();
```

### `milli`

#### Signature

```cpp
pluss::codem::Series milli(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `milli` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.milli();
```

### `millisecond`

#### Signature

```cpp
pluss::codem::Series millisecond(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `millisecond` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.millisecond();
```

### `milliseconds`

#### Signature

```cpp
pluss::codem::Series milliseconds(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `milliseconds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.milliseconds();
```

### `min`

#### Signature

```cpp
Timeline min(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `min` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.min();
```

### `minute`

#### Signature

```cpp
pluss::codem::Series minute(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `minute` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.minute();
```

### `minute_`

#### Signature

```cpp
pluss::codem::Series minute_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `minute`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.minute_();
```

### `minute_scalar`

#### Signature

```cpp
int minute_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `minute_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.minute_scalar();
```

### `minutes`

#### Signature

```cpp
pluss::codem::Series minutes(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `minutes` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.minutes();
```

### `minutes_between`

#### Signature

```cpp
int minutes_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `minutes_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.minutes_between(other_t);
```

### `mo`

#### Signature

```cpp
pluss::codem::Series mo(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `mo` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.mo();
```

### `mon`

#### Signature

```cpp
pluss::codem::Series mon(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `mon` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.mon();
```

### `mon_is`

#### Signature

```cpp
pluss::codem::Series mon_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `mon_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.mon_is();
```

### `month`

#### Signature

```cpp
pluss::codem::Series month(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `month` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.month();
```

### `month_`

#### Signature

```cpp
pluss::codem::Series month_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `month`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.month_();
```

### `month_name`

#### Signature

```cpp
pluss::coden::Note month_name(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Applies `month_name` to `self`. No additional arguments.

#### See also

- `std::string month_name(self, bool abbreviated) const`

#### Examples

```cpp
auto out = t.month_name();
```

### `month_name`

#### Signature

```cpp
std::string month_name(self, bool abbreviated) const
```

#### Parameters

`abbreviated` : `bool`
    Required method argument.

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `month_name` to `self`. Arguments: `bool abbreviated`.

#### See also

- `pluss::coden::Note month_name(self) const`

#### Examples

```cpp
auto out = t.month_name(true);
```

### `month_scalar`

#### Signature

```cpp
int month_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `month_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.month_scalar();
```

### `months`

#### Signature

```cpp
pluss::codem::Series months(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `months` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.months();
```

### `months_between`

#### Signature

```cpp
int months_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `months_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.months_between(other_t);
```

### `ms`

#### Signature

```cpp
pluss::codem::Series ms(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `ms` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.ms();
```

### `msec`

#### Signature

```cpp
pluss::codem::Series msec(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `msec` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.msec();
```

### `nano`

#### Signature

```cpp
pluss::codem::Series nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `nano` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.nano();
```

### `nanosecond`

#### Signature

```cpp
pluss::codem::Series nanosecond(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `nanosecond` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.nanosecond();
```

### `nanoseconds`

#### Signature

```cpp
pluss::codem::Series nanoseconds(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `nanoseconds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.nanoseconds();
```

### `ne`

#### Signature

```cpp
pluss::codem::Series ne(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `ne` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.ne(other_t);
```

### `normalize`

#### Signature

```cpp
Timeline normalize(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `normalize` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.normalize();
```

### `ns`

#### Signature

```cpp
pluss::codem::Series ns(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `ns` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.ns();
```

### `nsec`

#### Signature

```cpp
pluss::codem::Series nsec(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `nsec` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.nsec();
```

### `q`

#### Signature

```cpp
pluss::codem::Series q(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `q` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.q();
```

### `qtr`

#### Signature

```cpp
pluss::codem::Series qtr(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `qtr` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.qtr();
```

### `quarter`

#### Signature

```cpp
pluss::codem::Series quarter(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `quarter` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.quarter();
```

### `quarter_`

#### Signature

```cpp
pluss::codem::Series quarter_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `quarter`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.quarter_();
```

### `quarter_scalar`

#### Signature

```cpp
int quarter_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `quarter_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.quarter_scalar();
```

### `repeat`

#### Signature

```cpp
Timeline repeat(self, std::int64_t times) const
```

#### Parameters

`times` : `std::int64_t`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t times`.

#### Examples

```cpp
auto out = t.repeat(3);
```

### `round`

#### Signature

```cpp
Timeline round(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `round` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = t.round("D");
```

### `s`

#### Signature

```cpp
pluss::codem::Series s(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `s` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.s();
```

### `sat_is`

#### Signature

```cpp
pluss::codem::Series sat_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `sat_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.sat_is();
```

### `searchsorted_left`

#### Signature

```cpp
pluss::codem::Series searchsorted_left(self, const Timeline& values) const
```

#### Parameters

`values` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `searchsorted_left` to `self`. Arguments: `const Timeline& values`.

#### Examples

```cpp
auto out = t.searchsorted_left(other_t);
```

### `searchsorted_right`

#### Signature

```cpp
pluss::codem::Series searchsorted_right(self, const Timeline& values) const
```

#### Parameters

`values` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `searchsorted_right` to `self`. Arguments: `const Timeline& values`.

#### Examples

```cpp
auto out = t.searchsorted_right(other_t);
```

### `sec`

#### Signature

```cpp
pluss::codem::Series sec(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `sec` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.sec();
```

### `second`

#### Signature

```cpp
pluss::codem::Series second(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `second` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.second();
```

### `second_`

#### Signature

```cpp
pluss::codem::Series second_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `second`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.second_();
```

### `second_scalar`

#### Signature

```cpp
int second_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `second_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.second_scalar();
```

### `seconds`

#### Signature

```cpp
pluss::codem::Series seconds(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `seconds` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.seconds();
```

### `seconds_between`

#### Signature

```cpp
int seconds_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `seconds_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.seconds_between(other_t);
```

### `set_day`

#### Signature

```cpp
Timeline set_day(self, int day) const
```

#### Parameters

`day` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_day` to `self`. Arguments: `int day`.

#### Examples

```cpp
auto out = t.set_day(15);
```

### `set_hour`

#### Signature

```cpp
Timeline set_hour(self, int hour) const
```

#### Parameters

`hour` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_hour` to `self`. Arguments: `int hour`.

#### Examples

```cpp
auto out = t.set_hour(9);
```

### `set_minute`

#### Signature

```cpp
Timeline set_minute(self, int minute) const
```

#### Parameters

`minute` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_minute` to `self`. Arguments: `int minute`.

#### Examples

```cpp
auto out = t.set_minute(30);
```

### `set_month`

#### Signature

```cpp
Timeline set_month(self, int month) const
```

#### Parameters

`month` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_month` to `self`. Arguments: `int month`.

#### Examples

```cpp
auto out = t.set_month(12);
```

### `set_second`

#### Signature

```cpp
Timeline set_second(self, int second) const
```

#### Parameters

`second` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_second` to `self`. Arguments: `int second`.

#### Examples

```cpp
auto out = t.set_second(45);
```

### `set_year`

#### Signature

```cpp
Timeline set_year(self, int year) const
```

#### Parameters

`year` : `int`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `set_year` to `self`. Arguments: `int year`.

#### Examples

```cpp
auto out = t.set_year(2025);
```

### `shift`

#### Signature

```cpp
Timeline shift(self, int periods, std::string_view freq) const
```

#### Parameters

`periods` : `int`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `shift` to `self`. Arguments: `int periods, std::string_view freq`.

#### Examples

```cpp
auto out = t.shift(1, "D");
```

### `sort`

#### Signature

```cpp
Timeline sort(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = t.sort();
```

### `strftime`

#### Signature

```cpp
pluss::coden::Note strftime(self, std::string_view fmt) const
```

#### Parameters

`fmt` : `std::string_view`
    Required method argument.

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. Arguments: `std::string_view fmt`.

#### Examples

```cpp
auto out = t.strftime("%Y-%m-%d");
```

### `sub_duration`

#### Signature

```cpp
Timeline sub_duration(self, const Duration& d) const
```

#### Parameters

`d` : `const Duration&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `sub_duration` to `self`. Arguments: `const Duration& d`.

#### Examples

```cpp
auto out = t.sub_duration(dur);
```

### `sun_is`

#### Signature

```cpp
pluss::codem::Series sun_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `sun_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.sun_is();
```

### `thu_is`

#### Signature

```cpp
pluss::codem::Series thu_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `thu_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.thu_is();
```

### `time`

#### Signature

```cpp
Timeline time(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `time` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.time();
```

### `Timeline::createFromDateInts`

#### Signature

```cpp
Timeline::createFromDateInts(int year, int month, int day)
```

#### Parameters

`year` : `int`
    Required method argument.

`month` : `int`
    Required method argument.

`day` : `int`
    Required method argument.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `int year, int month, int day`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::createFromDateInts(2024, 1, 10);
```

### `Timeline::createFromDateTimeInts`

#### Signature

```cpp
Timeline::createFromDateTimeInts(int year, int month, int day, int hour, int minute, int second)
```

#### Parameters

`year` : `int`
    Required method argument.

`month` : `int`
    Required method argument.

`day` : `int`
    Required method argument.

`hour` : `int`
    Required method argument.

`minute` : `int`
    Required method argument.

`second` : `int`
    Required method argument.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `int year, int month, int day, int hour, int minute, int second`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::createFromDateTimeInts(2024, 1, 10, 8, 30, 0);
```

### `Timeline::createFromTimeInts`

#### Signature

```cpp
Timeline::createFromTimeInts(int hour, int minute, int second)
```

#### Parameters

`hour` : `int`
    Required method argument.

`minute` : `int`
    Required method argument.

`second` : `int`
    Required method argument.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `int hour, int minute, int second`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::createFromTimeInts(8, 30, 0);
```

### `Timeline::date_range`

#### Signature

```cpp
Timeline::date_range(const Timeline& start, const Timeline& end, std::string_view freq = "D", int step = 1)
```

#### Parameters

`start` : `const Timeline&`
    Required method argument.

`end` : `const Timeline&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument. Optional; default is `"D"`.

`step` : `int`
    Required method argument. Optional; default is `1`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static helper that creates a regular timeline range between start and end. Arguments: `const Timeline& start, const Timeline& end, std::string_view freq = "D", int step = 1`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::date_range(t, other_t, "D", 1);
```

### `Timeline::from_epoch_micros`

#### Signature

```cpp
Timeline::from_epoch_micros(const pluss::codem::Series& epoch_micros, std::string_view tz_name = "UTC")
```

#### Parameters

`epoch_micros` : `const pluss::codem::Series&`
    Required method argument.

`tz_name` : `std::string_view`
    Required method argument. Optional; default is `"UTC"`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_micros, std::string_view tz_name = "UTC"`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::from_epoch_micros(series_vals, "UTC");
```


### `Timeline::from_epoch_millis`

#### Signature

```cpp
Timeline::from_epoch_millis(const pluss::codem::Series& epoch_millis, std::string_view tz_name = "UTC")
```

#### Parameters

`epoch_millis` : `const pluss::codem::Series&`
    Required method argument.

`tz_name` : `std::string_view`
    Required method argument. Optional; default is `"UTC"`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_millis, std::string_view tz_name = "UTC"`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::from_epoch_millis(series_vals, "UTC");
```

### `Timeline::from_epoch_seconds`

#### Signature

```cpp
Timeline::from_epoch_seconds(const pluss::codem::Series& epoch_seconds, std::string_view tz_name = "UTC")
```

#### Parameters

`epoch_seconds` : `const pluss::codem::Series&`
    Required method argument.

`tz_name` : `std::string_view`
    Required method argument. Optional; default is `"UTC"`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& epoch_seconds, std::string_view tz_name = "UTC"`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::from_epoch_seconds(series_vals, "UTC");
```

### `Timeline::from_ymd`

#### Signature

```cpp
Timeline::from_ymd(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, std::string_view tz_name = "UTC")
```

#### Parameters

`year` : `const pluss::codem::Series&`
    Required method argument.

`month` : `const pluss::codem::Series&`
    Required method argument.

`day` : `const pluss::codem::Series&`
    Required method argument.

`tz_name` : `std::string_view`
    Required method argument. Optional; default is `"UTC"`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, std::string_view tz_name = "UTC"`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::from_ymd(year_col, month_col, day_col, "UTC");
```

### `Timeline::from_ymdhms`

#### Signature

```cpp
Timeline::from_ymdhms(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, const pluss::codem::Series& hour, const pluss::codem::Series& minute, const pluss::codem::Series& second, std::string_view tz_name = "UTC")
```

#### Parameters

`year` : `const pluss::codem::Series&`
    Required method argument.

`month` : `const pluss::codem::Series&`
    Required method argument.

`day` : `const pluss::codem::Series&`
    Required method argument.

`hour` : `const pluss::codem::Series&`
    Required method argument.

`minute` : `const pluss::codem::Series&`
    Required method argument.

`second` : `const pluss::codem::Series&`
    Required method argument.

`tz_name` : `std::string_view`
    Required method argument. Optional; default is `"UTC"`.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that creates a value from explicit input fields. Arguments: `const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, const pluss::codem::Series& hour, const pluss::codem::Series& minute, const pluss::codem::Series& second, std::string_view tz_name = "UTC"`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::from_ymdhms(year_col, month_col, day_col, hour_col, minute_col, second_col, "UTC");
```

### `Timeline::fromSeries`

#### Signature

```cpp
Timeline::fromSeries(const pluss::codem::Series& series)
```

#### Parameters

`series` : `const pluss::codem::Series&`
    Required method argument.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static helper/factory method. Arguments: `const pluss::codem::Series& series`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::fromSeries(series_vals);
```

### `Timeline::fromTimestamp`

#### Signature

```cpp
Timeline::fromTimestamp(std::int64_t timestamp_seconds)
```

#### Parameters

`timestamp_seconds` : `std::int64_t`
    Required method argument.

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static helper/factory method. Arguments: `std::int64_t timestamp_seconds`.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::fromTimestamp(1704067200);
```

### `Timeline::now`

#### Signature

```cpp
Timeline::now()
```

#### Parameters

`None`

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that returns a timeline anchored to `now`. No additional arguments.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::now();
```

### `Timeline::today`

#### Signature

```cpp
Timeline::today()
```

#### Parameters

`None`

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that returns a timeline anchored to `today`. No additional arguments.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::today();
```

### `Timeline::tomorrow`

#### Signature

```cpp
Timeline::tomorrow()
```

#### Parameters

`None`

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that returns a timeline anchored to `tomorrow`. No additional arguments.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::tomorrow();
```

### `Timeline::yesterday`

#### Signature

```cpp
Timeline::yesterday()
```

#### Parameters

`None`

#### Returns

`see signature`
    Return value produced by this method call.

#### Notes

Static constructor that returns a timeline anchored to `yesterday`. No additional arguments.

#### Examples

```cpp
auto out = pluss::codeh::Timeline::yesterday();
```

### `to_epoch_nanos_flat`

#### Signature

```cpp
std::vector<std::int64_t> to_epoch_nanos_flat(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<std::int64_t>`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = t.to_epoch_nanos_flat();
```

### `to_note_date`

#### Signature

```cpp
pluss::coden::Note to_note_date(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date();
```

### `to_note_date_compact`

#### Signature

```cpp
pluss::coden::Note to_note_date_compact(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_compact();
```

### `to_note_date_compact_nano`

#### Signature

```cpp
pluss::coden::Note to_note_date_compact_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_compact_nano();
```

### `to_note_date_compact_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_date_compact_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_compact_nano_z();
```

### `to_note_date_compact_z`

#### Signature

```cpp
pluss::coden::Note to_note_date_compact_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_compact_z();
```

### `to_note_date_nano`

#### Signature

```cpp
pluss::coden::Note to_note_date_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_nano();
```

### `to_note_date_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_date_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_nano_z();
```

### `to_note_date_z`

#### Signature

```cpp
pluss::coden::Note to_note_date_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_date_z();
```

### `to_note_datetime`

#### Signature

```cpp
pluss::coden::Note to_note_datetime(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_datetime();
```

### `to_note_datetime_nano`

#### Signature

```cpp
pluss::coden::Note to_note_datetime_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_datetime_nano();
```

### `to_note_datetime_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_datetime_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_datetime_nano_z();
```

### `to_note_datetime_z`

#### Signature

```cpp
pluss::coden::Note to_note_datetime_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_datetime_z();
```

### `to_note_dmy`

#### Signature

```cpp
pluss::coden::Note to_note_dmy(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmy();
```

### `to_note_dmY_hm_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dash();
```

### `to_note_dmY_hm_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dash_nano();
```

### `to_note_dmY_hm_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dash_nano_z();
```

### `to_note_dmY_hm_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dash_z();
```

### `to_note_dmY_hm_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dot();
```

### `to_note_dmY_hm_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dot_nano();
```

### `to_note_dmY_hm_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dot_nano_z();
```

### `to_note_dmY_hm_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_dot_z();
```

### `to_note_dmY_hm_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_slash();
```

### `to_note_dmY_hm_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_slash_nano();
```

### `to_note_dmY_hm_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_slash_nano_z();
```

### `to_note_dmY_hm_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_sp_slash_z();
```

### `to_note_dmY_hm_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dash();
```

### `to_note_dmY_hm_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dash_nano();
```

### `to_note_dmY_hm_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dash_nano_z();
```

### `to_note_dmY_hm_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dash_z();
```

### `to_note_dmY_hm_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dot();
```

### `to_note_dmY_hm_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dot_nano();
```

### `to_note_dmY_hm_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dot_nano_z();
```

### `to_note_dmY_hm_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_dot_z();
```

### `to_note_dmY_hm_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_slash();
```

### `to_note_dmY_hm_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_slash_nano();
```

### `to_note_dmY_hm_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_slash_nano_z();
```

### `to_note_dmY_hm_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hm_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hm_t_slash_z();
```

### `to_note_dmY_hms_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dash();
```

### `to_note_dmY_hms_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dash_nano();
```

### `to_note_dmY_hms_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dash_nano_z();
```

### `to_note_dmY_hms_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dash_z();
```

### `to_note_dmY_hms_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dot();
```

### `to_note_dmY_hms_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dot_nano();
```

### `to_note_dmY_hms_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dot_nano_z();
```

### `to_note_dmY_hms_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_dot_z();
```

### `to_note_dmY_hms_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_slash();
```

### `to_note_dmY_hms_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_slash_nano();
```

### `to_note_dmY_hms_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_slash_nano_z();
```

### `to_note_dmY_hms_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_sp_slash_z();
```

### `to_note_dmY_hms_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dash();
```

### `to_note_dmY_hms_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dash_nano();
```

### `to_note_dmY_hms_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dash_nano_z();
```

### `to_note_dmY_hms_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dash_z();
```

### `to_note_dmY_hms_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dot();
```

### `to_note_dmY_hms_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dot_nano();
```

### `to_note_dmY_hms_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dot_nano_z();
```

### `to_note_dmY_hms_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_dot_z();
```

### `to_note_dmY_hms_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_slash();
```

### `to_note_dmY_hms_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_slash_nano();
```

### `to_note_dmY_hms_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_slash_nano_z();
```

### `to_note_dmY_hms_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmY_hms_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmY_hms_t_slash_z();
```

### `to_note_dmy_nano`

#### Signature

```cpp
pluss::coden::Note to_note_dmy_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmy_nano();
```

### `to_note_dmy_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmy_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmy_nano_z();
```

### `to_note_dmy_z`

#### Signature

```cpp
pluss::coden::Note to_note_dmy_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_dmy_z();
```

### `to_note_iso`

#### Signature

```cpp
pluss::coden::Note to_note_iso(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_iso();
```

### `to_note_iso_week`

#### Signature

```cpp
pluss::coden::Note to_note_iso_week(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_iso_week();
```

### `to_note_iso_week_compact`

#### Signature

```cpp
pluss::coden::Note to_note_iso_week_compact(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_iso_week_compact();
```

### `to_note_md`

#### Signature

```cpp
pluss::coden::Note to_note_md(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_md();
```

### `to_note_md_nano`

#### Signature

```cpp
pluss::coden::Note to_note_md_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_md_nano();
```

### `to_note_md_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_md_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_md_nano_z();
```

### `to_note_md_z`

#### Signature

```cpp
pluss::coden::Note to_note_md_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_md_z();
```

### `to_note_mdY_hm_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dash();
```

### `to_note_mdY_hm_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dash_nano();
```

### `to_note_mdY_hm_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dash_nano_z();
```

### `to_note_mdY_hm_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dash_z();
```

### `to_note_mdY_hm_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dot();
```

### `to_note_mdY_hm_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dot_nano();
```

### `to_note_mdY_hm_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dot_nano_z();
```

### `to_note_mdY_hm_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_dot_z();
```

### `to_note_mdY_hm_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_slash();
```

### `to_note_mdY_hm_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_slash_nano();
```

### `to_note_mdY_hm_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_slash_nano_z();
```

### `to_note_mdY_hm_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_sp_slash_z();
```

### `to_note_mdY_hm_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dash();
```

### `to_note_mdY_hm_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dash_nano();
```

### `to_note_mdY_hm_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dash_nano_z();
```

### `to_note_mdY_hm_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dash_z();
```

### `to_note_mdY_hm_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dot();
```

### `to_note_mdY_hm_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dot_nano();
```

### `to_note_mdY_hm_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dot_nano_z();
```

### `to_note_mdY_hm_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_dot_z();
```

### `to_note_mdY_hm_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_slash();
```

### `to_note_mdY_hm_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_slash_nano();
```

### `to_note_mdY_hm_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_slash_nano_z();
```

### `to_note_mdY_hm_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hm_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hm_t_slash_z();
```

### `to_note_mdY_hms_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dash();
```

### `to_note_mdY_hms_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dash_nano();
```

### `to_note_mdY_hms_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dash_nano_z();
```

### `to_note_mdY_hms_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dash_z();
```

### `to_note_mdY_hms_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dot();
```

### `to_note_mdY_hms_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dot_nano();
```

### `to_note_mdY_hms_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dot_nano_z();
```

### `to_note_mdY_hms_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_dot_z();
```

### `to_note_mdY_hms_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_slash();
```

### `to_note_mdY_hms_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_slash_nano();
```

### `to_note_mdY_hms_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_slash_nano_z();
```

### `to_note_mdY_hms_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_sp_slash_z();
```

### `to_note_mdY_hms_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dash();
```

### `to_note_mdY_hms_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dash_nano();
```

### `to_note_mdY_hms_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dash_nano_z();
```

### `to_note_mdY_hms_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dash_z();
```

### `to_note_mdY_hms_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dot();
```

### `to_note_mdY_hms_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dot_nano();
```

### `to_note_mdY_hms_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dot_nano_z();
```

### `to_note_mdY_hms_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_dot_z();
```

### `to_note_mdY_hms_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_slash();
```

### `to_note_mdY_hms_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_slash_nano();
```

### `to_note_mdY_hms_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_slash_nano_z();
```

### `to_note_mdY_hms_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_mdY_hms_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_mdY_hms_t_slash_z();
```

### `to_note_month_name`

#### Signature

```cpp
pluss::coden::Note to_note_month_name(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_month_name();
```

### `to_note_month_name_long`

#### Signature

```cpp
pluss::coden::Note to_note_month_name_long(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_month_name_long();
```

### `to_note_month_year_name`

#### Signature

```cpp
pluss::coden::Note to_note_month_year_name(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_month_year_name();
```

### `to_note_my`

#### Signature

```cpp
pluss::coden::Note to_note_my(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_my();
```

### `to_note_my_nano`

#### Signature

```cpp
pluss::coden::Note to_note_my_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_my_nano();
```

### `to_note_my_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_my_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_my_nano_z();
```

### `to_note_my_z`

#### Signature

```cpp
pluss::coden::Note to_note_my_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_my_z();
```

### `to_note_rfc3339`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339();
```

### `to_note_rfc3339_nano`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339_nano();
```

### `to_note_rfc3339_nano_nano`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339_nano_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339_nano_nano();
```

### `to_note_rfc3339_nano_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339_nano_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339_nano_nano_z();
```

### `to_note_rfc3339_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339_nano_z();
```

### `to_note_rfc3339_z`

#### Signature

```cpp
pluss::coden::Note to_note_rfc3339_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_rfc3339_z();
```

### `to_note_time`

#### Signature

```cpp
pluss::coden::Note to_note_time(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time();
```

### `to_note_time_hm`

#### Signature

```cpp
pluss::coden::Note to_note_time_hm(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_hm();
```

### `to_note_time_hm_nano`

#### Signature

```cpp
pluss::coden::Note to_note_time_hm_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_hm_nano();
```

### `to_note_time_hm_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_time_hm_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_hm_nano_z();
```

### `to_note_time_hm_z`

#### Signature

```cpp
pluss::coden::Note to_note_time_hm_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_hm_z();
```

### `to_note_time_nano`

#### Signature

```cpp
pluss::coden::Note to_note_time_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_nano();
```

### `to_note_time_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_time_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_nano_z();
```

### `to_note_time_z`

#### Signature

```cpp
pluss::coden::Note to_note_time_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_time_z();
```

### `to_note_weekday`

#### Signature

```cpp
pluss::coden::Note to_note_weekday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_weekday();
```

### `to_note_weekday_long`

#### Signature

```cpp
pluss::coden::Note to_note_weekday_long(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_weekday_long();
```

### `to_note_ym`

#### Signature

```cpp
pluss::coden::Note to_note_ym(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_ym();
```

### `to_note_ym_nano`

#### Signature

```cpp
pluss::coden::Note to_note_ym_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_ym_nano();
```

### `to_note_ym_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_ym_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_ym_nano_z();
```

### `to_note_ym_z`

#### Signature

```cpp
pluss::coden::Note to_note_ym_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_ym_z();
```

### `to_note_Ymd_hm_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dash();
```

### `to_note_Ymd_hm_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dash_nano();
```

### `to_note_Ymd_hm_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dash_nano_z();
```

### `to_note_Ymd_hm_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dash_z();
```

### `to_note_Ymd_hm_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dot();
```

### `to_note_Ymd_hm_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dot_nano();
```

### `to_note_Ymd_hm_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dot_nano_z();
```

### `to_note_Ymd_hm_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_dot_z();
```

### `to_note_Ymd_hm_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_slash();
```

### `to_note_Ymd_hm_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_slash_nano();
```

### `to_note_Ymd_hm_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_slash_nano_z();
```

### `to_note_Ymd_hm_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_sp_slash_z();
```

### `to_note_Ymd_hm_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dash();
```

### `to_note_Ymd_hm_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dash_nano();
```

### `to_note_Ymd_hm_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dash_nano_z();
```

### `to_note_Ymd_hm_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dash_z();
```

### `to_note_Ymd_hm_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dot();
```

### `to_note_Ymd_hm_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dot_nano();
```

### `to_note_Ymd_hm_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dot_nano_z();
```

### `to_note_Ymd_hm_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_dot_z();
```

### `to_note_Ymd_hm_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_slash();
```

### `to_note_Ymd_hm_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_slash_nano();
```

### `to_note_Ymd_hm_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_slash_nano_z();
```

### `to_note_Ymd_hm_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hm_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hm_t_slash_z();
```

### `to_note_Ymd_hms_sp_dash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dash();
```

### `to_note_Ymd_hms_sp_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dash_nano();
```

### `to_note_Ymd_hms_sp_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dash_nano_z();
```

### `to_note_Ymd_hms_sp_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dash_z();
```

### `to_note_Ymd_hms_sp_dot`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dot();
```

### `to_note_Ymd_hms_sp_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dot_nano();
```

### `to_note_Ymd_hms_sp_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dot_nano_z();
```

### `to_note_Ymd_hms_sp_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_dot_z();
```

### `to_note_Ymd_hms_sp_slash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_slash();
```

### `to_note_Ymd_hms_sp_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_slash_nano();
```

### `to_note_Ymd_hms_sp_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_slash_nano_z();
```

### `to_note_Ymd_hms_sp_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_sp_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_sp_slash_z();
```

### `to_note_Ymd_hms_t_dash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dash();
```

### `to_note_Ymd_hms_t_dash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dash_nano();
```

### `to_note_Ymd_hms_t_dash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dash_nano_z();
```

### `to_note_Ymd_hms_t_dash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dash_z();
```

### `to_note_Ymd_hms_t_dot`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dot(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dot();
```

### `to_note_Ymd_hms_t_dot_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dot_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dot_nano();
```

### `to_note_Ymd_hms_t_dot_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dot_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dot_nano_z();
```

### `to_note_Ymd_hms_t_dot_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_dot_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_dot_z();
```

### `to_note_Ymd_hms_t_slash`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_slash(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_slash();
```

### `to_note_Ymd_hms_t_slash_nano`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_slash_nano(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_slash_nano();
```

### `to_note_Ymd_hms_t_slash_nano_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_slash_nano_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_slash_nano_z();
```

### `to_note_Ymd_hms_t_slash_z`

#### Signature

```cpp
pluss::coden::Note to_note_Ymd_hms_t_slash_z(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = t.to_note_Ymd_hms_t_slash_z();
```

### `to_period`

#### Signature

```cpp
Timeline to_period(self, std::string_view freq) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. Arguments: `std::string_view freq`.

#### Examples

```cpp
auto out = t.to_period("D");
```

### `to_Series`

#### Signature

```cpp
pluss::codem::Series to_Series(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = t.to_Series();
```

### `to_string`

#### Signature

```cpp
std::string to_string(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = t.to_string();
```

### `to_timeindex`

#### Signature

```cpp
TimeIndex to_timeindex(self, bool require_sorted) const
```

#### Parameters

`require_sorted` : `bool`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Builds a `TimeIndex` view from timeline values (optionally enforcing sortedness). Arguments: `bool require_sorted`.

#### Examples

```cpp
auto out = t.to_timeindex(true);
```

### `to_utc`

#### Signature

```cpp
Timeline to_utc(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = t.to_utc();
```

### `toISOString`

#### Signature

```cpp
std::string toISOString(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `toISOString` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.toISOString();
```

### `tue_is`

#### Signature

```cpp
pluss::codem::Series tue_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `tue_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.tue_is();
```

### `tz_convert`

#### Signature

```cpp
Timeline tz_convert(self, std::string_view tz_name) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `tz_convert` to `self`. Arguments: `std::string_view tz_name`.

#### Examples

```cpp
auto out = t.tz_convert("UTC");
```

### `tz_localize`

#### Signature

```cpp
Timeline tz_localize(self, std::string_view tz_name, int fold, std::string_view nonexistent) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

`fold` : `int`
    Required method argument.

`nonexistent` : `std::string_view`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `tz_localize` to `self`. Arguments: `std::string_view tz_name, int fold, std::string_view nonexistent`.

#### Examples

```cpp
auto out = t.tz_localize("UTC", 1, "raise");
```

### `unique`

#### Signature

```cpp
Timeline unique(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = t.unique();
```

### `us`

#### Signature

```cpp
pluss::codem::Series us(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `us` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.us();
```

### `usec`

#### Signature

```cpp
pluss::codem::Series usec(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `usec` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.usec();
```

### `wed_is`

#### Signature

```cpp
pluss::codem::Series wed_is(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `wed_is` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.wed_is();
```

### `week`

#### Signature

```cpp
pluss::codem::Series week(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `week` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.week();
```

### `week_`

#### Signature

```cpp
pluss::codem::Series week_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `week`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.week_();
```

### `week_of_year_scalar`

#### Signature

```cpp
int week_of_year_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `week_of_year_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.week_of_year_scalar();
```

### `weekday`

#### Signature

```cpp
pluss::codem::Series weekday(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `weekday` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.weekday();
```

### `weekday_`

#### Signature

```cpp
pluss::codem::Series weekday_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `weekday`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.weekday_();
```

### `weekday_name`

#### Signature

```cpp
pluss::coden::Note weekday_name(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Applies `weekday_name` to `self`. No additional arguments.

#### See also

- `std::string weekday_name(self, bool abbreviated) const`

#### Examples

```cpp
auto out = t.weekday_name();
```

### `weekday_name`

#### Signature

```cpp
std::string weekday_name(self, bool abbreviated) const
```

#### Parameters

`abbreviated` : `bool`
    Required method argument.

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `weekday_name` to `self`. Arguments: `bool abbreviated`.

#### See also

- `pluss::coden::Note weekday_name(self) const`

#### Examples

```cpp
auto out = t.weekday_name(true);
```

### `weekend`

#### Signature

```cpp
pluss::codem::Series weekend(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `weekend` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.weekend();
```

### `weekofyear`

#### Signature

```cpp
pluss::codem::Series weekofyear(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `weekofyear` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.weekofyear();
```

### `where`

#### Signature

```cpp
Timeline where(self, const pluss::codem::Series& mask, const Timeline& other) const
```

#### Parameters

`mask` : `const pluss::codem::Series&`
    Required method argument.

`other` : `const Timeline&`
    Required method argument.

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const pluss::codem::Series& mask, const Timeline& other`.

#### Examples

```cpp
auto out = t.where(mask, other_t);
```

### `woy`

#### Signature

```cpp
pluss::codem::Series woy(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `woy` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.woy();
```

### `y`

#### Signature

```cpp
pluss::codem::Series y(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `y` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.y();
```

### `year`

#### Signature

```cpp
pluss::codem::Series year(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `year` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.year();
```

### `year_`

#### Signature

```cpp
pluss::codem::Series year_(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

In-place variant of `year`; it mutates `self` and returns `self`. No additional arguments.

#### Examples

```cpp
auto out = t.year_();
```

### `year_scalar`

#### Signature

```cpp
int year_scalar(self) const
```

#### Parameters

`None`

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `year_scalar` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.year_scalar();
```

### `years`

#### Signature

```cpp
pluss::codem::Series years(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `years` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.years();
```

### `years_between`

#### Signature

```cpp
int years_between(self, const Timeline& other) const
```

#### Parameters

`other` : `const Timeline&`
    Required method argument.

#### Returns

`int`
    Return value produced by this method call.

#### Notes

Applies `years_between` to `self`. Arguments: `const Timeline& other`.

#### Examples

```cpp
auto out = t.years_between(other_t);
```

### `yr`

#### Signature

```cpp
pluss::codem::Series yr(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `yr` to `self`. No additional arguments.

#### Examples

```cpp
auto out = t.yr();
```

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


## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

### `align_union`

#### Signature

```cpp
AlignUnion align_union(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`AlignUnion`
    Return value produced by this method call.

#### Notes

Applies `align_union` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.align_union(other_idx);
```

### `argsort`

#### Signature

```cpp
pluss::codem::Series argsort(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.argsort();
```

### `asof`

#### Signature

```cpp
std::int64_t asof(self, const Timeline& key) const
```

#### Parameters

`key` : `const Timeline&`
    Required method argument.

#### Returns

`std::int64_t`
    Return value produced by this method call.

#### Notes

Applies `asof` to `self`. Arguments: `const Timeline& key`.

#### Examples

```cpp
auto out = idx.asof(key_t);
```

### `ceil`

#### Signature

```cpp
TimeIndex ceil(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `ceil` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = idx.ceil("D");
```

### `difference`

#### Signature

```cpp
TimeIndex difference(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `difference` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.difference(other_idx);
```

### `diffIndex`

#### Signature

```cpp
TimeIndex diffIndex(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `diffIndex` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.diffIndex(other_idx);
```

### `drop_duplicates`

#### Signature

```cpp
TimeIndex drop_duplicates(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `drop_duplicates` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.drop_duplicates();
```

### `floor`

#### Signature

```cpp
TimeIndex floor(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `floor` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = idx.floor("D");
```

### `format`

#### Signature

```cpp
pluss::coden::Note format(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = idx.format();
```

### `format_iso`

#### Signature

```cpp
pluss::coden::Note format_iso(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = idx.format_iso();
```

### `freq`

#### Signature

```cpp
std::string freq(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `freq` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.freq();
```

### `frequency`

#### Signature

```cpp
std::string frequency(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `frequency` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.frequency();
```

### `has_duplicates`

#### Signature

```cpp
bool has_duplicates(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = idx.has_duplicates();
```

### `head`

#### Signature

```cpp
TimeIndex head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `head` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto out = idx.head(1);
```

### `infer_freq`

#### Signature

```cpp
std::string infer_freq(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `infer_freq` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.infer_freq();
```

### `intersect`

#### Signature

```cpp
TimeIndex intersect(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `intersect` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.intersect(other_idx);
```

### `intersection`

#### Signature

```cpp
TimeIndex intersection(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `intersection` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.intersection(other_idx);
```

### `is_monotonic_decreasing`

#### Signature

```cpp
bool is_monotonic_decreasing(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = idx.is_monotonic_decreasing();
```

### `is_monotonic_increasing`

#### Signature

```cpp
bool is_monotonic_increasing(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = idx.is_monotonic_increasing();
```

### `is_unique`

#### Signature

```cpp
bool is_unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto out = idx.is_unique();
```

### `max`

#### Signature

```cpp
Timeline max(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `max` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.max();
```

### `min`

#### Signature

```cpp
Timeline min(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `min` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.min();
```

### `normalize`

#### Signature

```cpp
TimeIndex normalize(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `normalize` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.normalize();
```

### `resample_agg`

#### Signature

```cpp
pluss::codem::Series resample_agg(self, const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`op` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_agg(values, "H", "sum", 1);
```

### `resample_bins`

#### Signature

```cpp
ResampleBins resample_bins(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`ResampleBins`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_bins("H", 1);
```

### `resample_count`

#### Signature

```cpp
pluss::codem::Series resample_count(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_count("H", 1);
```

### `resample_first`

#### Signature

```cpp
pluss::codem::Series resample_first(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_first(values, "H", 1);
```

### `resample_last`

#### Signature

```cpp
pluss::codem::Series resample_last(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_last(values, "H", 1);
```

### `resample_max`

#### Signature

```cpp
pluss::codem::Series resample_max(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_max(values, "H", 1);
```

### `resample_mean`

#### Signature

```cpp
pluss::codem::Series resample_mean(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_mean(values, "H", 1);
```

### `resample_min`

#### Signature

```cpp
pluss::codem::Series resample_min(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_min(values, "H", 1);
```

### `resample_ohlc`

#### Signature

```cpp
pluss::codem::Series resample_ohlc(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_ohlc(values, "H", 1);
```

### `resample_size`

#### Signature

```cpp
pluss::codem::Series resample_size(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_size("H", 1);
```

### `resample_sum`

#### Signature

```cpp
pluss::codem::Series resample_sum(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto out = idx.resample_sum(values, "H", 1);
```

### `rolling_agg`

#### Signature

```cpp
pluss::codem::Series rolling_agg(self, const pluss::codem::Series& values, const Duration& window, std::string_view op) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

`op` : `std::string_view`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window, std::string_view op`.

#### Examples

```cpp
auto out = idx.rolling_agg(values, dur, "sum");
```

### `rolling_count`

#### Signature

```cpp
pluss::codem::Series rolling_count(self, const Duration& window) const
```

#### Parameters

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_count(dur);
```

### `rolling_first`

#### Signature

```cpp
pluss::codem::Series rolling_first(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_first(values, dur);
```

### `rolling_last`

#### Signature

```cpp
pluss::codem::Series rolling_last(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_last(values, dur);
```

### `rolling_max`

#### Signature

```cpp
pluss::codem::Series rolling_max(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_max(values, dur);
```

### `rolling_mean`

#### Signature

```cpp
pluss::codem::Series rolling_mean(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_mean(values, dur);
```

### `rolling_min`

#### Signature

```cpp
pluss::codem::Series rolling_min(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_min(values, dur);
```

### `rolling_sum`

#### Signature

```cpp
pluss::codem::Series rolling_sum(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto out = idx.rolling_sum(values, dur);
```

### `round`

#### Signature

```cpp
TimeIndex round(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `round` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto out = idx.round("D");
```

### `searchsorted`

#### Signature

```cpp
std::int64_t searchsorted(self, const Timeline& key, bool right) const
```

#### Parameters

`key` : `const Timeline&`
    Required method argument.

`right` : `bool`
    Required method argument.

#### Returns

`std::int64_t`
    Return value produced by this method call.

#### Notes

Applies `searchsorted` to `self`. Arguments: `const Timeline& key, bool right`.

#### Examples

```cpp
auto out = idx.searchsorted(key_t, true);
```

### `slice`

#### Signature

```cpp
pluss::codem::Series slice(self, const Timeline& start, const Timeline& end) const
```

#### Parameters

`start` : `const Timeline&`
    Required method argument.

`end` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `slice` to `self`. Arguments: `const Timeline& start, const Timeline& end`.

#### Examples

```cpp
auto out = idx.slice(key_t, key_t);
```

### `slice_index`

#### Signature

```cpp
TimeIndex slice_index(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Required method argument.

`stop` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `slice_index` to `self`. Arguments: `std::int64_t start, std::int64_t stop`.

#### Examples

```cpp
auto out = idx.slice_index(1, 1);
```

### `slice_time`

#### Signature

```cpp
pluss::codem::Series slice_time(self, std::string_view start_iso, std::string_view end_iso) const
```

#### Parameters

`start_iso` : `std::string_view`
    Required method argument.

`end_iso` : `std::string_view`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `slice_time` to `self`. Arguments: `std::string_view start_iso, std::string_view end_iso`.

#### Examples

```cpp
auto out = idx.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");
```

### `sort`

#### Signature

```cpp
TimeIndex sort(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.sort();
```

### `sort_index`

#### Signature

```cpp
TimeIndex sort_index(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `sort_index` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.sort_index();
```

### `sort_values`

#### Signature

```cpp
TimeIndex sort_values(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `sort_values` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.sort_values();
```

### `tail`

#### Signature

```cpp
TimeIndex tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tail` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto out = idx.tail(1);
```

### `take`

#### Signature

```cpp
TimeIndex take(self, const pluss::codem::Series& indices) const
```

#### Parameters

`indices` : `const pluss::codem::Series&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const pluss::codem::Series& indices`.

#### Examples

```cpp
auto out = idx.take(indices);
```

### `to_note_iso`

#### Signature

```cpp
pluss::coden::Note to_note_iso(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto out = idx.to_note_iso();
```

### `to_string`

#### Signature

```cpp
pluss::coden::Note to_string(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = idx.to_string();
```

### `to_utc`

#### Signature

```cpp
TimeIndex to_utc(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto out = idx.to_utc();
```

### `tostring`

#### Signature

```cpp
pluss::coden::Note tostring(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Applies `tostring` to `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.tostring();
```

### `tz_convert`

#### Signature

```cpp
TimeIndex tz_convert(self, std::string_view tz_name) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tz_convert` to `self`. Arguments: `std::string_view tz_name`.

#### Examples

```cpp
auto out = idx.tz_convert("UTC");
```

### `tz_localize`

#### Signature

```cpp
TimeIndex tz_localize(self, std::string_view tz_name, int fold, std::string_view nonexistent) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

`fold` : `int`
    Required method argument.

`nonexistent` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tz_localize` to `self`. Arguments: `std::string_view tz_name, int fold, std::string_view nonexistent`.

#### Examples

```cpp
auto out = idx.tz_localize("UTC", 1, "raise");
```

### `union_`

#### Signature

```cpp
TimeIndex union_(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

In-place variant of `union`; it mutates `self` and returns `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.union_(other_idx);
```

### `unionIndex`

#### Signature

```cpp
TimeIndex unionIndex(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `unionIndex` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto out = idx.unionIndex(other_idx);
```

### `unique`

#### Signature

```cpp
TimeIndex unique(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto out = idx.unique();
```

## Demos to run

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`
