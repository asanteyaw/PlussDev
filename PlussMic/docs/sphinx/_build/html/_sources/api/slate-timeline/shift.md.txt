<!-- pluss-hand-authored -->

# SlateTimelineAccessor::shift

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const -> Slate
```

## What It Does

Shifts every row of the bound slate forward (or backward, if `periods` is
negative) by `periods * freq` — same length, still `Timeline`-kind.
Month/year shifts are calendar-aware: a day-of-month that doesn't exist in the
target month is clamped down to that month's last valid day (never rolls over
into the next month).

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

`Slate`
    A new `Timeline`-kind slate, same length as the bound slate,
    shifted by `periods * freq`.

## Examples

Example 1. Shift forward one calendar month, showing day-of-month clamping.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:00:00Z",
    "2024-02-29T09:00:00Z",
}));
auto out = events.tl().shift(1, "months");
std::cout << out;
```

### Output

```text
0  2024-02-29 09:00:00
1  2024-03-29 09:00:00

ptype: Timeline
```

January 31st shifted one month forward lands on February 29th (2024 is a leap
year) — day `31` is clamped down to February's last valid day, `29`, rather
than overflowing into March.

Example 2. Shift forward one calendar **year** across a leap boundary.

```cpp
auto leap_day = Slate(pluss::make_timeline({"2024-02-29T09:00:00Z"}));
auto out2 = leap_day.tl().shift(1, "years");
std::cout << out2;
```

### Output

```text
0  2025-02-28 09:00:00

ptype: Timeline
```

`2024` is a leap year but `2025` isn't, so `February 29th` has no equivalent
one year later — it clamps down to `2025-02-28`.

Example 3. Shift forward by `"days"`, which is exact elapsed-time arithmetic
with no clamping — contrast with the calendar-aware `months`/`years` above.

```cpp
auto plain = Slate(pluss::make_timeline({"2024-01-30T09:00:00Z"}));
auto out3 = plain.tl().shift(3, "days");
std::cout << out3;
```

### Output

```text
0  2024-02-02 09:00:00

ptype: Timeline
```

Example 4. A negative `periods` moves backward, crossing both an hour and a
day boundary in one call.

```cpp
auto ts = Slate(pluss::make_timeline({"2024-01-02T00:20:00Z"}));
auto out4 = ts.tl().shift(-90, "minutes");
std::cout << out4;
```

### Output

```text
0  2024-01-01 22:50:00

ptype: Timeline
```

Example 5. An unrecognized frequency string throws.

```cpp
try {
  auto bad = ts.tl().shift(1, "week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::shift: unsupported frequency
```

## Notes

- `freq` accepts calendar units (`years`/`months`) and fixed units
  (`days`/`hours`/`minutes`/`seconds`) — there's no `"week"` frequency.
- Calendar-unit shifts (`years`/`months`) clamp the day-of-month rather than
  overflowing into a later month; fixed-unit shifts (`days` and finer) are
  exact elapsed-time arithmetic with no clamping concern.
- `freq` cannot be empty, and an unrecognized string throws.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` regardless of `periods`/`freq`.

```{note} Distinct from Slate::shift()
`Slate` also has a direct, unrelated `shift(periods = 1)` overload (a plain
positional lag/lead, not calendar-aware). This accessor method is a
completely different operation, reached only via `.tl()`.
```

## See Also

- [`SlateTimelineAccessor::period`](period.md): truncate to the *start* of a
  calendar period instead of shifting by a fixed number of them.

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
