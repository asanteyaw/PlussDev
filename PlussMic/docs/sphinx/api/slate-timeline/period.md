<!-- pluss-hand-authored -->

# SlateTimelineAccessor::period

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::period(std::string_view freq) const -> Slate
```

## What It Does

Truncates every row of the bound slate to the *start* of its enclosing `freq`
period — same length, still `Timeline`-kind.

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

`Slate`
    A new `Timeline`-kind slate, same length as the bound slate,
    truncated to the start of its `freq` period.

## Examples

Example 1. Truncate every row to its month start.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().period("months");
std::cout << out;
```

### Output

```text
0  2024-01-01
1  2024-02-01
2  2024-07-01

ptype: Timeline
```

Example 2. Truncate the same rows to their year start instead.

```cpp
auto out2 = events.tl().period("years");
std::cout << out2;
```

### Output

```text
0  2024-01-01
1  2024-01-01
2  2024-01-01

ptype: Timeline
```

All three rows collapse to `2024-01-01`, regardless of their original month or
day — `period("years")` only cares which year a row falls in.

Example 3. Truncate to the day — identical to `floor("D")` under a different
name.

```cpp
auto single = Slate(pluss::make_timeline({"2024-06-01T14:05:00Z"}));
auto out3 = single.tl().period("days");
std::cout << out3;
```

### Output

```text
0  2024-06-01

ptype: Timeline
```

`"hours"`, `"minutes"`, and `"seconds"` delegate the same way, to
`floor("H")`/`floor("min")`/`floor("S")` respectively.

Example 4. An unrecognized frequency string throws.

```cpp
try {
  auto bad = single.tl().period("week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline::to_period: unsupported frequency
```

## Notes

- Unlike [`shift()`](shift.md), `period()` doesn't move by a count of
  periods — it always snaps down to the *start* of the row's own current
  period; combine with [`shift()`](shift.md) for "start of the next month."
- `"day"`/`"hour"`/`"minute"`/`"second"` truncation is equivalent to
  [`floor()`](floor.md) with the corresponding unit; `"year"`/`"month"` go
  further than `floor()` supports (`floor()` has no calendar-unit option).
- `freq` cannot be empty, and an unrecognized string throws.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`SlateTimelineAccessor::shift`](shift.md): move by a count of calendar or
  fixed periods instead of truncating to the current one.
- [`SlateTimelineAccessor::floor`](floor.md): the sub-day-unit equivalent
  this method delegates to for `day`/`hour`/`minute`/`second`.

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
