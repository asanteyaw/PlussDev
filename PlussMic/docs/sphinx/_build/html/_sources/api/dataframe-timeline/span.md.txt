<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: efe2b15e584b -->

# DataFrameTimelineAccessor::span

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signatures

```cpp
DataFrameTimelineAccessor::span(const DataFrame& other, const TimelineSpanOptions& options = {}) const -> DataFrame
DataFrameTimelineAccessor::span(const DataFrame& other, TimeUnit unit) const -> DataFrame
DataFrameTimelineAccessor::span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const -> DataFrame
DataFrameTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const -> DataFrame
DataFrameTimelineAccessor::span(const Slate& other, const TimelineSpanOptions& options = {}) const -> DataFrame
DataFrameTimelineAccessor::span(const Slate& other, TimeUnit unit) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with the elapsed
distance between it and `other`, in `options.unit` (or plain `unit`) — same
feature name, now a numeric `Series`. `other` can be another `DataFrame`
(paired feature-by-feature, by matching name), a bare `Slate`, or a
`pluss::Timeline`.

Fixed-duration units (`Nanos`/`Micros`/`Millis`/`Seconds`/`Days`/`Weeks`) are
plain elapsed-time differences. Calendar units (`Months`/`Years`) count
completed calendar periods or crossed calendar boundaries, per
`options.mode`. `BusinessDays` counts business days per `options.calendar`
(default: Monday-Friday, no holidays).

## Parameters

**other** (`const DataFrame&` / `const Slate&` / `const pluss::Timeline&`)

The right-hand timestamp(s). For the `DataFrame` overload, every selected
feature name on `*this` must also exist on `other`; each is paired by name.

**options** (`const TimelineSpanOptions&`)

`unit` (default `TimeUnit::Days`), `mode` (`Complete` — fully elapsed calendar
units, default; or `Boundary` — crossed calendar labels; only affects
`Months`/`Years`), `anchor` (`DayOfMonth` — strict day-of-month completion,
default; or `MonthEnd` — treat matching month-end dates as equivalent),
`signed_result` (default `true` — negative when `other` precedes `*this`;
`false` returns the absolute magnitude), and `calendar` (a `BusinessCalendar`,
only used for `BusinessDays`).

**unit** (`TimeUnit`, convenience overload)

Shorthand for `TimelineSpanOptions{.unit = unit}` with every other option left
at its default.

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by an `Int64` Series
    of elapsed distances to `other`.

## Examples

Example 1. Fixed-duration span in whole days.

```cpp
using namespace pluss::table;

auto starts = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T00:00:00Z",
          "2024-02-29T00:00:00Z",
          "2024-01-01T00:00:00Z",
      }))}});
auto ends = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-02-01T00:00:00Z",
          "2025-02-28T00:00:00Z",
          "2024-01-05T00:00:00Z",
      }))}});
std::cout << starts;
std::cout << ends;

auto out = starts.tl().span(ends, TimeUnit::Days);
std::cout << out;
```

### Output (starts)

```text
=================
     | ts        
-----------------
0    | 2024-01-31
1    | 2024-02-29
2    | 2024-01-01
-----------------
[3 cs x 1 feat]
```

### Output (ends)

```text
=================
     | ts        
-----------------
0    | 2024-02-01
1    | 2025-02-28
2    | 2024-01-05
-----------------
[3 cs x 1 feat]
```

### Output (span, Days)

```text
===========
     | ts  
-----------
0    | 1   
1    | 365 
2    | 4   
-----------
[3 cs x 1 feat]
```

`TimeUnit::Days` is a fixed 24-hour unit — `"2024-02-29"` (a leap day) to
`"2025-02-28"` is exactly `365` fixed days, the same as any other
non-leap-affected 365-day span.

Example 2. Calendar-month span: `Complete` vs `Boundary` mode.

```cpp
auto start = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}))}});
auto end   = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-02-01T00:00:00Z"}))}});

TimelineSpanOptions months;
months.unit = TimeUnit::Months;
months.mode = TimelineSpanMode::Complete;
auto complete = start.tl().span(end, months);
std::cout << complete;

months.mode = TimelineSpanMode::Boundary;
auto boundary = start.tl().span(end, months);
std::cout << boundary;
```

### Output (Complete)

```text
===========
     | ts  
-----------
0    | 0   
-----------
[1 cs x 1 feat]
```

### Output (Boundary)

```text
===========
     | ts  
-----------
0    | 1   
-----------
[1 cs x 1 feat]
```

January 31st to February 1st isn't a *fully completed* calendar month
(`Complete` = `0`, since February 1st's day-of-month is behind January
31st's), but it does cross one month-label boundary (`Boundary` = `1`).

Example 3. Use `signed_result = false` to get the magnitude when `other`
precedes `*this`.

```cpp
using namespace pluss::table;

auto self  = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-02-01T00:00:00Z"}))}});
auto other = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});
std::cout << self;
std::cout << other;

auto signed_out = self.tl().span(other, TimeUnit::Days);
std::cout << signed_out;

TimelineSpanOptions unsigned_opts;
unsigned_opts.unit = TimeUnit::Days;
unsigned_opts.signed_result = false;
auto unsigned_out = self.tl().span(other, unsigned_opts);
std::cout << unsigned_out;
```

### Output (self)

```text
=================
     | ts        
-----------------
0    | 2024-02-01
-----------------
[1 cs x 1 feat]
```

### Output (other)

```text
=================
     | ts        
-----------------
0    | 2024-01-01
-----------------
[1 cs x 1 feat]
```

### Output (signed_result = true, the default)

```text
===========
     | ts  
-----------
0    | -31 
-----------
[1 cs x 1 feat]
```

### Output (signed_result = false)

```text
===========
     | ts  
-----------
0    | 31  
-----------
[1 cs x 1 feat]
```

`other` (`2024-01-01`) is 31 days *before* `self` (`2024-02-01`), so the
default signed result is negative, `-31`. With `signed_result = false`, the
same span comes back as the plain magnitude, `31`.

Example 4. Pass `other` as a bare `Slate` or a raw `pluss::Timeline`, instead
of wrapping it in a `DataFrame`.

```cpp
using namespace pluss::table;

auto self = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});

auto via_slate = self.tl().span(Slate(pluss::make_timeline({"2024-02-01T00:00:00Z"})), TimeUnit::Days);
std::cout << via_slate;

auto via_timeline = self.tl().span(pluss::make_timeline({"2024-02-01T00:00:00Z"}), TimeUnit::Days);
std::cout << via_timeline;
```

### Output (Slate overload)

```text
===========
     | ts  
-----------
0    | 31  
-----------
[1 cs x 1 feat]
```

### Output (pluss::Timeline overload)

```text
===========
     | ts  
-----------
0    | 31  
-----------
[1 cs x 1 feat]
```

Both overloads produce the exact same result as pairing two `DataFrame`s by
feature name — they're just more convenient when `other` is a single
timestamp (or a bare column) rather than a whole frame.

Example 5. Count `BusinessDays` between two Mondays-through-Fridays, with and
without a holiday in the range.

```cpp
using namespace pluss::table;

auto self  = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});
auto other = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-12T00:00:00Z"}))}});
std::cout << self;
std::cout << other;

TimelineSpanOptions plain;
plain.unit = TimeUnit::BusinessDays;
plain.calendar = BusinessCalendar::monfri();
auto plain_out = self.tl().span(other, plain);
std::cout << plain_out;

TimelineSpanOptions with_holiday;
with_holiday.unit = TimeUnit::BusinessDays;
with_holiday.calendar = BusinessCalendar::monfri();
with_holiday.calendar.holidays.insert(19731); // 2024-01-09, a Tuesday
auto holiday_out = self.tl().span(other, with_holiday);
std::cout << holiday_out;
```

### Output (self)

```text
=================
     | ts        
-----------------
0    | 2024-01-01
-----------------
[1 cs x 1 feat]
```

### Output (other)

```text
=================
     | ts        
-----------------
0    | 2024-01-12
-----------------
[1 cs x 1 feat]
```

### Output (no holidays)

```text
===========
     | ts  
-----------
0    | 9   
-----------
[1 cs x 1 feat]
```

### Output (2024-01-09 marked as a holiday)

```text
===========
     | ts  
-----------
0    | 8   
-----------
[1 cs x 1 feat]
```

`2024-01-01` is a Monday and `2024-01-12` is the second Friday after it; with
the default `closed = Right` (start excluded, end included), that's `9`
weekdays (`01-02` through `01-12`, skipping the `01-06`/`01-07` weekend).
Marking `2024-01-09` (a Tuesday) as a holiday removes it from the count,
leaving `8`. Holiday entries are local epoch-day numbers — `19731` is
`2024-01-09`'s epoch-day label, the same kind of value
[`epoch(TimeUnit::Days)`](epoch.md) produces.

Example 6. Use `anchor = TimelineSpanAnchor::MonthEnd` so two month-end dates
count as a completed month, even though their days-of-month don't match.

```cpp
using namespace pluss::table;

auto self  = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}))}});
auto other = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"}))}});
std::cout << self;
std::cout << other;

TimelineSpanOptions day_of_month; // default anchor
day_of_month.unit = TimeUnit::Months;
auto strict_out = self.tl().span(other, day_of_month);
std::cout << strict_out;

TimelineSpanOptions month_end;
month_end.unit = TimeUnit::Months;
month_end.anchor = TimelineSpanAnchor::MonthEnd;
auto month_end_out = self.tl().span(other, month_end);
std::cout << month_end_out;
```

### Output (self)

```text
=================
     | ts        
-----------------
0    | 2024-01-31
-----------------
[1 cs x 1 feat]
```

### Output (other)

```text
=================
     | ts        
-----------------
0    | 2024-02-29
-----------------
[1 cs x 1 feat]
```

### Output (anchor = DayOfMonth, the default)

```text
===========
     | ts  
-----------
0    | 0   
-----------
[1 cs x 1 feat]
```

### Output (anchor = MonthEnd)

```text
===========
     | ts  
-----------
0    | 1   
-----------
[1 cs x 1 feat]
```

`January 31st` and `February 29th` are each their own month's last day, but
`29 < 31`, so the default `DayOfMonth` anchor doesn't consider the month
fully complete (`0`). With `anchor = MonthEnd`, both being month-end dates is
treated as equivalent to a matching day-of-month, so the same pair counts as
one completed month.

Example 7. Span a calendar unit (`Months`) between two timelines with
mismatched timezones, to see the call throw.

```cpp
using namespace pluss::table;

auto self  = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}))}});
auto other = DataFrame({{"ts", Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"}))}})
                 .tl().tz("America/New_York");

try {
  auto out = self.tl().span(other, TimeUnit::Months);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::span: calendar/business spans require matching timezones; convert first
```

`self` is `"UTC"` and `other` was relabeled to `"America/New_York"` — for a
calendar unit like `Months` (or `Years`/`BusinessDays`), both sides must share
the same timezone before the span can be computed; convert one side with
[`tz()`](tz.md) first. Fixed-duration units (`Days`, `Seconds`, and so on, as
in Example 1) have no such requirement.

## Notes

- Calendar units (`Months`/`Years`) and `BusinessDays` require `*this` and
  `other` to share the same timezone — mismatched timezones throw; convert
  one side with [`tz()`](tz.md) first.
- `anchor = TimelineSpanAnchor::MonthEnd` changes `Complete`-mode completion
  so that two month-end dates (e.g. January 31st and February 29th in a leap
  year) count as equivalent, rather than requiring an exact day-of-month
  match.
- `BusinessDays` uses `options.calendar` (default: Monday-Friday, no
  holidays); built-in presets include `BusinessCalendar::monfri()`,
  `usa_federal(...)`, `canada_federal(...)`, `uk_bank(...)`, `target2(...)`,
  and `nyse(...)`. Holidays are local epoch-day labels (e.g. from
  `.tl().epoch(TimeUnit::Days)`), not epoch nanoseconds. See the manual's
  "Timeline spans" section for worked `BusinessDays` examples.
- `signed_result = false` returns the absolute magnitude instead of a
  direction-sensitive (possibly negative) value.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- If either side is `NaT` for a row, that row's span displays as `"NULL"`.

## See Also

- [`DataFrameTimelineAccessor::epoch`](epoch.md): a fixed-unit distance from
  the Unix epoch to a single timeline, rather than between two timelines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
