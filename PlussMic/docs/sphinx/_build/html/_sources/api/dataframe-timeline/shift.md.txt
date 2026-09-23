<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 66362b45516d -->

# DataFrameTimelineAccessor::shift

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with each row shifted
forward (or backward, if `periods` is negative) by `periods * freq` — same
feature name, still `Timeline`/`TimeIndex`-kind, same row order.
Month/year shifts are calendar-aware: a day-of-month that doesn't exist in
the target month is clamped down to that month's last valid day (never
rolls over into the next month).

## Parameters

**periods** (`std::int64_t`)

How many `freq` units to shift by. Positive moves forward in time, negative
moves backward.

**freq** (`std::string_view`)

One of `"y"`/`"yr"`/`"year"`/`"years"`/`"annual"`,
`"mo"`/`"mth"`/`"month"`/`"months"`, `"d"`/`"day"`/`"days"`,
`"h"`/`"hr"`/`"hour"`/`"hours"`, `"t"`/`"min"`/`"minute"`/`"minutes"`, or
`"s"`/`"sec"`/`"second"`/`"seconds"` (case-insensitive). Cannot be empty.

## Returns

`DataFrame`
    A new frame with each selected time feature shifted by `periods * freq`.

## Examples

Example 1. Shift forward one calendar month, showing day-of-month clamping.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:00:00Z",
          "2024-02-29T09:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().shift(1, "months");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:00:00
1    | 2024-02-29 09:00:00
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-02-29 09:00:00
1    | 2024-03-29 09:00:00
--------------------------
[2 cs x 1 feat]
```

January 31st shifted one month forward lands on February 29th (2024 is a leap
year) — day `31` is clamped down to February's last valid day, `29`, rather
than overflowing into March. February 29th shifted one month forward lands
cleanly on March 29th, since March has at least 29 days.

Example 2. Shift forward one calendar **year** across a leap boundary, to see
the same day-of-month clamp apply at year granularity.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-02-29T09:00:00Z"}))}});
std::cout << events;

auto out = events.tl().shift(1, "years");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-02-29 09:00:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2025-02-28 09:00:00
--------------------------
[1 cs x 1 feat]
```

`2024` is a leap year but `2025` isn't, so `February 29th` has no equivalent
one year later — it clamps down to `2025-02-28`, the same clamping rule
`months` uses, just applied across a year.

Example 3. Shift forward by `"days"`, which is exact elapsed-time arithmetic
with no clamping — contrast with the calendar-aware `months`/`years` above.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-30T09:00:00Z"}))}});
std::cout << events;

auto out = events.tl().shift(3, "days");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-30 09:00:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-02-02 09:00:00
--------------------------
[1 cs x 1 feat]
```

`January 30 + 3 days` simply counts three whole days forward and rolls into
the next month — `February 2nd` — with no day-of-month clamping logic
involved at all.

Example 4. Shift by `"hours"` in both directions to see `periods` go negative.

```cpp
using namespace pluss::table;

auto forward_base = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-01T23:00:00Z"}))}});
auto backward_base = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-02T01:00:00Z"}))}});

auto forward_out = forward_base.tl().shift(3, "hours");
std::cout << forward_out;

auto backward_out = backward_base.tl().shift(-3, "hours");
std::cout << backward_out;
```

### Output (forward, periods = 3)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-02 02:00:00
--------------------------
[1 cs x 1 feat]
```

### Output (backward, periods = -3)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-01 22:00:00
--------------------------
[1 cs x 1 feat]
```

`23:00` on `01-01` plus 3 hours crosses midnight forward into `01-02 02:00`.
`01:00` on `01-02` minus 3 hours crosses midnight backward into `01-01 22:00`
— a negative `periods` moves strictly backward, with the same fixed-elapsed
arithmetic (no clamping) as the positive case.

Example 5. Shift backward by `"minutes"`, crossing both an hour and a day
boundary in one call.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-02T00:20:00Z"}))}});
std::cout << events;

auto out = events.tl().shift(-90, "minutes");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-02 00:20:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-01 22:50:00
--------------------------
[1 cs x 1 feat]
```

90 minutes before `01-02 00:20` lands at `01-01 22:50` — crossing back over
both the hour and the calendar-day boundary in a single elapsed-time
subtraction. `"seconds"` follows the exact same fixed-elapsed mechanism, just
one level finer-grained than `"minutes"`.

Example 6. Pass an unrecognized frequency string to see the call throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});

try {
  auto out = events.tl().shift(1, "week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::shift: unsupported frequency
```

`"week"` isn't one of the accepted year/month/day/hour/minute/second
frequency groups, so the call throws instead of picking a nearby unit.

## Notes

- `freq` accepts calendar units (`years`/`months`) and fixed units
  (`days`/`hours`/`minutes`/`seconds`) — there's no `"week"` frequency.
- Calendar-unit shifts (`years`/`months`) clamp the day-of-month rather than
  overflowing into a later month; fixed-unit shifts (`days` and finer) are
  exact elapsed-time arithmetic with no clamping concern.
- `freq` cannot be empty, and an unrecognized string throws
  (`"Timeline::shift: unsupported frequency"`).
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` regardless of `periods`/`freq`.

## See Also

- [`DataFrameTimelineAccessor::period`](period.md): truncate to the *start*
  of a calendar period instead of shifting by a fixed number of them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html">
    <span class="label">Previous</span>
    <span class="title">second</span>
  </a>
  <a class="next-link" href="span.html">
    <span class="label">Next</span>
    <span class="title">span</span>
  </a>
</div>
