<!-- pluss-hand-authored -->

# SlateTimelineAccessor::span

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signatures

```cpp
SlateTimelineAccessor::span(const Slate& other, const TimelineSpanOptions& options = {}) const -> Slate
SlateTimelineAccessor::span(const Slate& other, TimeUnit unit) const -> Slate
SlateTimelineAccessor::span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const -> Slate
SlateTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const -> Slate
```

## What It Does

Replaces the bound slate with the elapsed distance between it and `other`, in
`options.unit` (or plain `unit`) — same length, now a numeric `Series`.
`other` can be another `Slate` or a bare `pluss::Timeline` scalar.

Fixed-duration units (`Nanos`/`Micros`/`Millis`/`Seconds`/`Days`/`Weeks`) are
plain elapsed-time differences. Calendar units (`Months`/`Years`) count
completed calendar periods or crossed calendar boundaries, per
`options.mode`. `BusinessDays` counts business days per `options.calendar`
(default: Monday-Friday, no holidays).

## Parameters

**other** (`const Slate&` / `const pluss::Timeline&`)

The right-hand timestamp(s), paired row-by-row with the bound slate.

**options** (`const TimelineSpanOptions&`)

`unit` (default `TimeUnit::Days`), `mode` (`Complete` — fully elapsed calendar
units, default; or `Boundary` — crossed calendar labels; only affects
`Months`/`Years`), `anchor` (`DayOfMonth` — strict day-of-month completion,
default; or `MonthEnd` — treat matching month-end dates as equivalent),
`signed_result` (default `true` — negative when `other` precedes the bound
slate; `false` returns the absolute magnitude), and `calendar` (a
`BusinessCalendar`, only used for `BusinessDays`).

**unit** (`TimeUnit`, convenience overload)

Shorthand for `TimelineSpanOptions{.unit = unit}` with every other option left
at its default.

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's elapsed distance to `other`.

## Examples

Example 1. Fixed-duration span in whole days.

```cpp
using namespace pluss::table;

auto starts = Slate(pluss::make_timeline({
    "2024-01-31T00:00:00Z",
    "2024-02-29T00:00:00Z",
    "2024-01-01T00:00:00Z",
}));
auto ends = Slate(pluss::make_timeline({
    "2024-02-01T00:00:00Z",
    "2025-02-28T00:00:00Z",
    "2024-01-05T00:00:00Z",
}));
auto out = starts.tl().span(ends, TimeUnit::Days);
std::cout << out;
```

### Output

```text
0  1
1  365
2  4

ptype: Series
```

`TimeUnit::Days` is a fixed 24-hour unit — `"2024-02-29"` (a leap day) to
`"2025-02-28"` is exactly `365` fixed days.

Example 2. Calendar-month span: `Complete` vs `Boundary` mode.

```cpp
auto start = Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}));
auto end = Slate(pluss::make_timeline({"2024-02-01T00:00:00Z"}));

TimelineSpanOptions months;
months.unit = TimeUnit::Months;
months.mode = TimelineSpanMode::Complete;
std::cout << start.tl().span(end, months);

months.mode = TimelineSpanMode::Boundary;
std::cout << start.tl().span(end, months);
```

### Output (Complete)

```text
0  0

ptype: Series
```

### Output (Boundary)

```text
0  1

ptype: Series
```

January 31st to February 1st isn't a *fully completed* calendar month
(`Complete` = `0`), but it does cross one month-label boundary
(`Boundary` = `1`).

Example 3. Use `signed_result = false` to get the magnitude when `other`
precedes `*this`.

```cpp
auto self_ts = Slate(pluss::make_timeline({"2024-02-01T00:00:00Z"}));
auto other_ts = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << self_ts.tl().span(other_ts, TimeUnit::Days);

TimelineSpanOptions unsigned_opts;
unsigned_opts.unit = TimeUnit::Days;
unsigned_opts.signed_result = false;
std::cout << self_ts.tl().span(other_ts, unsigned_opts);
```

### Output (signed_result = true, the default)

```text
0  -31

ptype: Series
```

### Output (signed_result = false)

```text
0  31

ptype: Series
```

`other_ts` (`2024-01-01`) is 31 days *before* `self_ts` (`2024-02-01`), so the
default signed result is negative, `-31`.

Example 4. Pass `other` as a raw `pluss::Timeline` scalar instead of a
`Slate`.

```cpp
auto self2 = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));
auto out4 = self2.tl().span(pluss::make_timeline({"2024-02-01T00:00:00Z"}), TimeUnit::Days);
std::cout << out4;
```

### Output

```text
0  31

ptype: Series
```

Example 5. Count `BusinessDays` between two dates, with and without a holiday
in the range.

```cpp
auto biz_start = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));
auto biz_end = Slate(pluss::make_timeline({"2024-01-12T00:00:00Z"}));

TimelineSpanOptions plain;
plain.unit = TimeUnit::BusinessDays;
plain.calendar = BusinessCalendar::monfri();
std::cout << biz_start.tl().span(biz_end, plain);

TimelineSpanOptions with_holiday;
with_holiday.unit = TimeUnit::BusinessDays;
with_holiday.calendar = BusinessCalendar::monfri();
with_holiday.calendar.holidays.insert(19731); // 2024-01-09, a Tuesday
std::cout << biz_start.tl().span(biz_end, with_holiday);
```

### Output (no holidays)

```text
0  9

ptype: Series
```

### Output (2024-01-09 marked as a holiday)

```text
0  8

ptype: Series
```

`2024-01-01` is a Monday and `2024-01-12` is the second Friday after it; with
the default `closed = Right`, that's `9` weekdays. Marking `2024-01-09` (a
Tuesday) as a holiday removes it from the count, leaving `8`. Holiday entries
are local epoch-day numbers — `19731` is `2024-01-09`'s epoch-day label, the
same kind of value [`epoch(TimeUnit::Days)`](epoch.md) produces.

Example 6. Use `anchor = TimelineSpanAnchor::MonthEnd` so two month-end dates
count as a completed month, even though their days-of-month don't match.

```cpp
auto anchor_self = Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}));
auto anchor_other = Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"}));

TimelineSpanOptions day_of_month; // default anchor
day_of_month.unit = TimeUnit::Months;
std::cout << anchor_self.tl().span(anchor_other, day_of_month);

TimelineSpanOptions month_end;
month_end.unit = TimeUnit::Months;
month_end.anchor = TimelineSpanAnchor::MonthEnd;
std::cout << anchor_self.tl().span(anchor_other, month_end);
```

### Output (anchor = DayOfMonth, the default)

```text
0  0

ptype: Series
```

### Output (anchor = MonthEnd)

```text
0  1

ptype: Series
```

`January 31st` and `February 29th` are each their own month's last day, but
`29 < 31`, so the default `DayOfMonth` anchor doesn't consider the month fully
complete (`0`). With `anchor = MonthEnd`, both being month-end dates is
treated as equivalent to a matching day-of-month, so the pair counts as one
completed month.

Example 7. Span a calendar unit (`Months`) between two timelines with
mismatched timezones, to see the call throw.

```cpp
auto tz_self = Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}));
auto tz_other = Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"})).tl().tz("America/New_York");

try {
  auto bad = tz_self.tl().span(tz_other, TimeUnit::Months);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::span: calendar/business spans require matching timezones; convert first
```

`tz_self` is `"UTC"` and `tz_other` was relabeled to `"America/New_York"` —
for a calendar unit like `Months` (or `Years`/`BusinessDays`), both sides must
share the same timezone before the span can be computed; convert one side
with [`tz()`](tz.md) first. Fixed-duration units (`Days`, `Seconds`, and so
on, as in Example 1) have no such requirement.

## Notes

- Calendar units (`Months`/`Years`) and `BusinessDays` require the bound
  slate and `other` to share the same timezone — mismatched timezones throw;
  convert one side with [`tz()`](tz.md) first.
- `anchor = TimelineSpanAnchor::MonthEnd` changes `Complete`-mode completion
  so that two month-end dates (e.g. January 31st and February 29th in a leap
  year) count as equivalent, rather than requiring an exact day-of-month
  match.
- `BusinessDays` uses `options.calendar` (default: Monday-Friday, no
  holidays); built-in presets include `BusinessCalendar::monfri()`,
  `usa_federal(...)`, `canada_federal(...)`, `uk_bank(...)`, `target2(...)`,
  and `nyse(...)`. Holidays are local epoch-day labels (e.g. from
  `.tl().epoch(TimeUnit::Days)`), not epoch nanoseconds.
- `signed_result = false` returns the absolute magnitude instead of a
  direction-sensitive (possibly negative) value.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- If either side is `NaT` for a row, that row's span displays as `"NULL"`.

## See Also

- [`SlateTimelineAccessor::epoch`](epoch.md): a fixed-unit distance from the
  Unix epoch to a single timeline, rather than between two timelines.

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
