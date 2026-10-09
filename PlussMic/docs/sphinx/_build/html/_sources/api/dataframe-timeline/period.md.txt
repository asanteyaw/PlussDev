<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e224bcb5fed6 -->

# DataFrameTimelineAccessor::period

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::period(std::string_view freq) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with each row
truncated to the *start* of its enclosing `freq` period — same feature name,
still `Timeline`-kind, same row order.

## Parameters

**freq** (`std::string_view`)

One of `"y"`/`"yr"`/`"year"`/`"years"`/`"annual"` (truncate to January 1st,
00:00:00 of the row's year), `"mo"`/`"mth"`/`"month"`/`"months"` (truncate to
the 1st of the row's month, 00:00:00), `"d"`/`"day"`/`"days"` (floor to local
midnight), `"h"`/`"hr"`/`"hour"`/`"hours"` (floor to the hour),
`"t"`/`"min"`/`"minute"`/`"minutes"` (floor to the minute), or
`"s"`/`"sec"`/`"second"`/`"seconds"` (floor to the second) — case-insensitive.
Cannot be empty.

## Returns

`DataFrame`
    A new frame with each selected time feature truncated to the start of its
    `freq` period.

## Examples

Example 1. Truncate every row of a `Timeline` feature to its month start.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().period("months");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-01-01
1    | 2024-02-01
2    | 2024-07-01
3    | 2023-03-01
-----------------
[4 cs x 1 feat]
```

Every row lands on the 1st of its own month, with the time-of-day dropped.

Example 2. Truncate the same rows to their year start instead.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().period("years");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-01-01
1    | 2024-01-01
2    | 2024-01-01
3    | 2023-01-01
-----------------
[4 cs x 1 feat]
```

The three `2024` rows all collapse to the same `2024-01-01`, regardless of
their original month or day; the `2023` row collapses to `2023-01-01`.
`period("years")` only cares which year a row falls in.

Example 3. Truncate to the day, confirming it's exactly `floor("D")` under a
different name.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-06-01T14:05:00Z"}))}});
std::cout << events;

auto out = events.tl().period("days");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-06-01
-----------------
[1 cs x 1 feat]
```

This is identical to calling `events.tl().floor("D")` directly — `"hours"`,
`"minutes"`, and `"seconds"` delegate the same way, to `floor("H")`/
`floor("min")`/`floor("S")` respectively; see [`floor()`](floor.md) for those
three worked at every granularity.

Example 4. Pass an unrecognized frequency string to see the call throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-06-01T14:05:00Z"}))}});

try {
  auto out = events.tl().period("week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::to_period: unsupported frequency
```

`"week"` isn't one of the accepted year/month/day/hour/minute/second
frequency groups, so the call throws rather than guessing at a coarser
period.

## Notes

- Unlike [`shift()`](shift.md), `period()` doesn't move by a count of
  periods — it always snaps down to the *start* of the row's own current
  period (there's no way to ask for, say, the start of the *next* month
  directly; combine with [`shift()`](shift.md) for that).
- `"day"`/`"hour"`/`"minute"`/`"second"` truncation is equivalent to
  [`floor()`](floor.md) with the corresponding unit; `"year"`/`"month"` go
  further than `floor()` supports (`floor()` has no calendar-unit option).
- `freq` cannot be empty, and an unrecognized string throws.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`DataFrameTimelineAccessor::shift`](shift.md): move by a count of calendar
  or fixed periods instead of truncating to the current one.
- [`DataFrameTimelineAccessor::floor`](floor.md): the sub-day-unit
  equivalent this method delegates to for `day`/`hour`/`minute`/`second`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_start.html">
    <span class="label">Previous</span>
    <span class="title">month_start</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
