<!-- pluss-hand-authored -->

# Timeline::span

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::span(
    const Timeline& other,
    const TimelineSpanOptions& options = {}) const
```

## Measure From This Timeline To Another

`span()` counts temporal units from the receiver to `other`. The receiver is
the start and `other` is the end. It returns signed 64-bit integer counts in a
`Series`, preserving the broadcasted shape of the two temporal operands.

This method exists because “distance in time” has several valid meanings:

- elapsed nanoseconds, seconds, hours, or exact 24-hour days
- completed calendar months or years
- crossed month or year labels
- working dates selected by a weekday and holiday calendar

Those meanings cannot be inferred from one integer argument. `TimelineSpanOptions`
makes the choice explicit and keeps unit counting separate from
[`diff()`](diff.md), which always returns the exact elapsed distance as a
`Duration`.

The default options count complete elapsed 24-hour days and preserve direction.
Each field has a deliberately narrow role:

| Field | Meaning | Default | Applies to |
| --- | --- | --- | --- |
| `unit` | Selects the elapsed, calendar, or business unit to count. | `TimelineSpanUnit::Days` | Every call |
| `mode` | Chooses completed units or crossed calendar labels. | `TimelineSpanMode::Complete` | `Months`, `Years` |
| `anchor` | Decides whether month ends correspond when a strict day number does not exist. | `TimelineSpanAnchor::DayOfMonth` | Complete `Months`, complete `Years` |
| `signed_result` | Preserves direction when `true`; returns the non-negative magnitude when `false`. | `true` | Every unit |
| `calendar` | Supplies the working-week mask, holiday dates, and optional timezone constraint. | Monday-Friday, no holidays, no timezone constraint | `BusinessDays` |
| `closed` | Selects which endpoint dates are eligible for business-day counting. | `TimelineSpanClosed::Right` | `BusinessDays` |

Fields outside the selected unit's “Applies to” column are ignored. In
particular, changing `mode`, `anchor`, `calendar`, or `closed` cannot alter a
fixed elapsed-unit result.

## Fixed Elapsed Units

The fixed units are:

| Unit | Exact divisor |
| --- | ---: |
| `Nanos` | 1 ns |
| `Micros` | 1,000 ns |
| `Millis` | 1,000,000 ns |
| `Seconds` | 1,000,000,000 ns |
| `Minutes` | 60 seconds |
| `Hours` | 3,600 seconds |
| `Days` | 86,400 seconds |
| `Weeks` | 604,800 seconds |

For these units, `span()` subtracts epoch instants and divides by the selected
unit. An incomplete unit is discarded by integer truncation toward zero. It
does not round to the nearest unit.

Example 1. Default options count one complete elapsed day in a distance of one
day, one hour, two minutes, three seconds, and 210 nanoseconds.

```cpp
const auto start =
    pluss::timeline::make("2024-01-01T00:00:00.123456789Z");
const auto end =
    pluss::timeline::make("2024-01-02T01:02:03.123456999Z");
std::cout << pluss::series::print(start.span(end)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 2. Select a fixed unit by changing only `unit`. Each result is an
integer count from the same exact difference.

```cpp
auto options = pluss::TimelineSpanOptions{};

options.unit = pluss::TimelineSpanUnit::Nanos;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Micros;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Millis;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Seconds;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Minutes;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Hours;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
options.unit = pluss::TimelineSpanUnit::Weeks;
std::cout << pluss::series::print(start.span(end, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=90123000000210
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=90123000000
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=90123000
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=90123
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1502
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=25
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

## Elapsed Days Are Not Civil Days

`TimelineSpanUnit::Days` means an exact 86,400-second divisor. It does not count
date labels. A pair of equal local clock readings on consecutive dates can be
23 or 25 elapsed hours apart when the UTC offset changes.

Example 3. The next civil day across this spring transition is 23 elapsed
hours away. It therefore contains zero complete elapsed `Days`.

```cpp
const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");
const auto next_civil_day = spring.add_days(1);
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Hours;
std::cout << pluss::series::print(spring.span(next_civil_day, options)) << '\n';
std::cout << pluss::series::print(spring.span(next_civil_day)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=23
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

## Direction And Absolute Magnitude

With the default `signed_result=true`, reversing the operands reverses the
sign. Set `signed_result=false` when direction is irrelevant and only the
non-negative magnitude is wanted. Absolute conversion happens before range
validation, without relying on negating the most-negative 64-bit value.

Example 4. The same reverse span can be signed or absolute.

```cpp
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Seconds;
std::cout << pluss::series::print(end.span(start, options)) << '\n';

options.signed_result = false;
std::cout << pluss::series::print(end.span(start, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=-90123
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=90123
```

## Broadcasting And Output Shape

The two timelines follow trailing-axis broadcasting. A scalar pairs with every
element of an array; equal shapes pair elementwise; singleton dimensions can
expand. Incompatible dimensions are rejected. The output is a new contiguous
`int64` `Series` with the broadcasted shape.

Example 5. A scalar start is measured against two end instants. The second
distance is two complete days because the extra twelve hours are truncated.

```cpp
const auto scalar = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto vector = pluss::timeline::make({
    "2024-01-02T00:00:00Z",
    "2024-01-03T12:00:00Z",
});
std::cout << pluss::series::print(scalar.span(vector)) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
```

Example 6. Broadcasting can expand both operands. Shapes `[2, 1]` and `[1, 2]`
produce a `[2, 2]` matrix of pairwise day counts.

```cpp
const std::vector<std::string> column_values{
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
const std::vector<std::string> row_values{
    "2024-01-03T00:00:00Z", "2024-01-05T00:00:00Z"};
const auto column = pluss::timeline::make(column_values, {2, 1});
const auto row = pluss::timeline::make(row_values, {1, 2});
std::cout << pluss::series::print(column.span(row)) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 2 4 ⎤
⎣ 1 3 ⎦
```

## Completed Calendar Months

`TimelineSpanUnit::Months` uses local calendar fields rather than a fixed
duration. With `mode=Complete`, a month is counted only after the destination
has reached the required local day and time of day.

The default `anchor=DayOfMonth` is strict: January 31 to February 29 is not a
complete month because the destination never reaches day 31. Set
`anchor=MonthEnd` when the final valid day of each month should be treated as
the corresponding monthly position.

Month-end equivalence does not discard clock precision. If the destination is
one nanosecond before the source clock, the month is still incomplete.

Example 7. The three complete-month outcomes isolate day anchoring and the
time-of-day boundary.

```cpp
const auto jan31_noon = pluss::timeline::make("2024-01-31T12:00:00Z");
const auto feb29_early =
    pluss::timeline::make("2024-02-29T11:59:59.999999999Z");
const auto feb29_noon = pluss::timeline::make("2024-02-29T12:00:00Z");

auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Months;
std::cout << pluss::series::print(jan31_noon.span(feb29_noon, options)) << '\n';

options.anchor = pluss::TimelineSpanAnchor::MonthEnd;
std::cout << pluss::series::print(jan31_noon.span(feb29_early, options)) << '\n';
std::cout << pluss::series::print(jan31_noon.span(feb29_noon, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

## Calendar Boundaries Instead Of Complete Units

`mode=Boundary` counts changes in the calendar label. For months, it computes
the difference between year/month labels; for years, it computes the difference
between year labels. The day and time do not need to reach an anniversary.
`anchor` has no effect in boundary mode.

Example 8. January to February crosses one month boundary even though the end
is one nanosecond short of the month-end clock anchor.

```cpp
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Months;
options.mode = pluss::TimelineSpanMode::Boundary;
std::cout << pluss::series::print(jan31_noon.span(feb29_early, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

## Completed Calendar Years

`TimelineSpanUnit::Years` follows the same policies at annual scale. Complete
mode requires the local anniversary date and clock. `MonthEnd` anchoring treats
the final day of February in a non-leap year as corresponding to leap day;
strict `DayOfMonth` anchoring does not.

Example 9. The anchor determines whether this leap-day interval contains one
complete calendar year.

```cpp
const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
const auto next_feb_end = pluss::timeline::make("2025-02-28T12:00:00Z");
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Years;
std::cout << pluss::series::print(leap.span(next_feb_end, options)) << '\n';

options.anchor = pluss::TimelineSpanAnchor::MonthEnd;
std::cout << pluss::series::print(leap.span(next_feb_end, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

## Business-Day Counting

Select `BusinessDays` to count local calendar dates admitted by a
`BusinessCalendar`. A calendar contains:

- a seven-bit `weekmask`, using Sunday as bit 0 through Saturday as bit 6
- local epoch-day holiday labels to exclude
- descriptive `name` and `region` metadata
- an optional required timezone

`BusinessCalendar::monfri()` admits Monday through Friday and has no holidays.
Built-in constructors provide `usa_federal`, `canada_federal`, `uk_bank`,
`target2`, and `nyse` holiday sets for an inclusive year range. Choose a range
that covers every endpoint you will measure; dates outside that generated
range still follow the weekmask but have no generated holiday exclusions.

The `closed` option determines which endpoint dates can be counted:

| Value | Interval |
| --- | --- |
| `Right` | `(start, end]` |
| `Left` | `[start, end)` |
| `Both` | `[start, end]` |
| `Neither` | `(start, end)` |

Closure applies to local dates, not clock instants. Weekend and holiday dates
remain excluded even when closed. `mode` and `anchor` do not alter business-day
counting.

Example 10. From Friday to Monday, each closure policy selects a different
combination of endpoint dates. Weekend dates remain excluded in every case.

```cpp
const auto friday = pluss::timeline::make("2024-07-05T15:00:00Z");
const auto monday = pluss::timeline::make("2024-07-08T09:00:00Z");
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::BusinessDays;
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';

options.closed = pluss::TimelineSpanClosed::Both;
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';
options.closed = pluss::TimelineSpanClosed::Left;
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';
options.closed = pluss::TimelineSpanClosed::Neither;
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=2
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 11. A custom holiday is supplied as a local epoch-day label. Marking
the Monday endpoint as a holiday reduces the right-closed result to zero.

```cpp
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::BusinessDays;
options.calendar.holidays =
    pluss::timeline::date("2024-07-08")
        .epoch_days()
        .to_vec<std::int64_t>();
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 12. A custom weekmask can define a different working week. This mask
admits Saturday only, so the open-start, closed-end interval from Friday to
Monday counts the intervening Saturday.

```cpp
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::BusinessDays;
options.calendar.weekmask = 0b1000000;
std::cout << pluss::series::print(friday.span(monday, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 13. A generated federal calendar excludes the Monday holiday, while a
plain weekday calendar counts both Monday and Tuesday.

```cpp
const auto before = pluss::timeline::make("2024-06-28T12:00:00Z");
const auto after = pluss::timeline::make("2024-07-02T12:00:00Z");
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::BusinessDays;
options.calendar = pluss::BusinessCalendar::canada_federal(2024, 2024);
std::cout << pluss::series::print(before.span(after, options)) << '\n';

options.calendar = pluss::BusinessCalendar::monfri();
std::cout << pluss::series::print(before.span(after, options)) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=2
```

## Timezone Rules

Fixed elapsed units compare stored instants, so the two timelines may carry
different timezone labels. Months, years, and business days read local dates;
they require matching timezone names to prevent an implicit choice of calendar.
A business calendar with a non-empty `timezone` must also match the timeline.
Convert explicitly with [`tz_convert()`](tz_convert.md) before measuring when
calendar labels must be interpreted in a chosen zone.

Example 14. A calendar span with unlike timezone labels is rejected even when
both values identify the same instant.

```cpp
try {
  const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto local = utc.tz_convert("America/Edmonton");
  auto options = pluss::TimelineSpanOptions{};
  options.unit = pluss::TimelineSpanUnit::Months;
  (void)utc.span(local, options);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## Absent Instants, Empty Shapes, And Strides

If either paired instant is `NaT`, the corresponding integer result is
`Timeline::kNaT`. This sentinel is the minimum signed 64-bit value and prints
as `-9223372036854775808` in the returned numeric `Series`. Other pairs are
still measured normally.

Empty broadcast-compatible shapes return an empty `Series` with the broadcasted
shape. Non-contiguous timeline inputs are read by logical strides rather than
as packed storage.

Example 15. Absence propagates elementwise.

```cpp
const auto values = pluss::timeline::make({"2024-01-01", ""});
std::cout << pluss::series::print(
    values.span(pluss::timeline::date("2024-01-03"))) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, -9223372036854775808]
```

Example 16. A zero-length leading dimension remains part of the result shape.

```cpp
const auto empty = pluss::timeline::epoch({0, 2});
std::cout << pluss::series::print(
    empty.span(pluss::timeline::from_epoch(0LL))) << '\n';
```

### Output

```text
Series(shape=[0,2], strides=[2,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## Validation And Range Safety

`span()` rejects:

- non-broadcastable timeline shapes
- calendar or business spans with different timezone names
- an empty business weekmask or bits outside the seven weekdays
- a timezone-constrained business calendar used with another timezone
- holiday entries that are `Timeline::kNaT` or look like epoch nanoseconds
- a final count that cannot fit in signed `int64`

Subtraction is widened before division. This matters near the epoch-storage
limits: a seconds quotient can fit even when the raw nanosecond difference does
not. A nanosecond count for that same pair is rejected instead of wrapping.

Example 17. The full-range seconds quotient is representable, but the
nanosecond count is not.

```cpp
const auto near_min = pluss::timeline::from_epoch(
    std::numeric_limits<std::int64_t>::min() + 1);
const auto near_max = pluss::timeline::from_epoch(
    std::numeric_limits<std::int64_t>::max());
auto options = pluss::TimelineSpanOptions{};
options.unit = pluss::TimelineSpanUnit::Seconds;
std::cout << pluss::series::print(near_min.span(near_max, options)) << '\n';

try {
  options.unit = pluss::TimelineSpanUnit::Nanos;
  (void)near_min.span(near_max, options);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=18446744073
rejected
```

## Choosing The Right Measurement

Use `span()` when the result must be an integer count under an explicit unit or
calendar policy. Use [`diff()`](diff.md) when you need the exact elapsed
`Duration`, want duration arithmetic afterward, or do not want truncation.
Use the `add_*` family when the task is to construct shifted timestamps rather
than measure existing pairs.

## See Also

- [`Timeline::diff`](diff.md): exact elapsed distance as a `Duration`.
- [`Timeline::add_days`](add_days.md): move by civil days rather than measure elapsed days.
- [`Timeline::tz_convert`](tz_convert.md): choose the calendar timezone explicitly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="strftime.html">
    <span class="label">Next</span>
    <span class="title">strftime</span>
  </a>
</div>
