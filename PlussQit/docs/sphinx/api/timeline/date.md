# Timeline::date

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::date() const
```

## Project Onto Local Calendar Dates

`date()` extracts each instant's local year, month, and day and represents that
date as local midnight. A `Timeline` cannot store a date without a clock, so
midnight is the canonical instant representation rather than formatting text.
Use it when the visible civil date is the identity; use `epoch_days()` for fixed
UTC 86,400-second bins.

For day-sized flooring, `floor("D")` produces the same ticks. The distinction is
intent: `date()` is the component projection used by
`pluss::timeline::date(existing_timeline)`, while `floor()` belongs to the
parameterized rounding family alongside hour, minute, and second boundaries.

## Result Contract

The result is a newly allocated Timeline with unchanged rank, shape, timezone, ordering, and `NaT` positions. The receiver is not modified. Scalar, vector, matrix, three-dimensional, and empty shapes are all preserved.

## Example 1: Shape And Absence

```cpp
const auto times = pluss::timeline::make({
    "2024-07-15T18:34:56.123456789Z", "",
}).tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(times.date()) << '\n';
```

```text
Timeline(shape=[2], tz=America/Edmonton)
[ 2024-07-15 00:00:00, NaT ]
```

## Example 2: Timezone And Precision

```cpp
const auto noon = pluss::timeline::make(
    "2024-03-10T18:00:00Z").tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(noon.date()) << '\n';
```

```text
Timeline(shape=[], tz=America/Edmonton)
2024-03-10 00:00:00
```

## Choosing The Projection

Distinct clock times on one local date collapse to the same result. Midnight is resolved using its own offset, not the source time's offset. A nonexistent local midnight is rejected.

Every defined Timeline instant has a date. A value originally built from
clock-only text uses the documented `1970-01-01` anchor, so its extracted date
is that anchor; there is no hidden partial-date marker on which to return an
empty result. `NaT` lanes remain `NaT` without changing surrounding geometry.

`time()` keeps the complementary clock fields. Neither method formats text or retains enough information to reverse the projection.

## See Also

- [`time`](time.md)
- [`floor`](floor.md)
- [`epoch_days`](epoch_days.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copy_ticks.html"><span class="label">Previous</span><span class="title">copy_ticks</span></a>
  <a class="next-link" href="date_range.html"><span class="label">Next</span><span class="title">date_range</span></a>
</div>
<!-- pluss-hand-authored -->
