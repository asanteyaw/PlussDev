# Timeline::is_year_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_year_start() const
```

## What Counts As A Year Start

`is_year_start()` marks every instant on January 1 in the Timeline's local
civil calendar. The local time can be midnight, noon, or any other time on that
date. The predicate does not infer fiscal calendars and does not inspect which
observation appears first in storage.

## Result Contract

The result is a newly allocated boolean `Series` with the same rank and shape
as the receiver. Scalars remain rank zero, matrices remain matrices, empty
extents remain empty, and `NaT` becomes `false`. The receiver is not changed.
Timezone metadata matters because it determines the local month and day.

## Example 1: Mixed Calendar Dates

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_year_start());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, false, false, true, false, false]
```

Leap-year status changes the year's length, not its opening date.

## Example 2: Shape And Time-Of-Day

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_year_start());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡  true false false ⎤
⎣ false false false ⎦
```

January 1 remains a year start through its final nanosecond.

## Example 3: Select The Reporting Timezone First

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_year_start().item<bool>() << '\n'
          << "New York=" << local.is_year_start().item<bool>() << '\n';
```

```text
UTC=true
New York=false
```

The instant is January 1 in UTC but December 31 in New York.

## Example 4: Year Starts Are Quarter Starts

```cpp
const auto starts = dates.is_year_start();
std::cout << std::boolalpha
          << starts.logical_and(dates.is_quarter_start()).equal(starts) << '\n';
```

```text
true
```

## Gotchas And Failure Modes

- This is civil January 1, not a configurable fiscal-year boundary.
- Every timestamp on January 1 qualifies; the method does not choose one row.
- `false` does not establish validity because `NaT` is also false.
- The operation supports every Timeline rank and performs no reduction.

## See Also

- [`is_year_end`](is_year_end.md)
- [`is_quarter_start`](is_quarter_start.md)
- [`year`](year.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_year_end.html"><span class="label">Previous</span><span class="title">is_year_end</span></a>
  <a class="next-link" href="isnat.html"><span class="label">Next</span><span class="title">isnat</span></a>
</div>
<!-- pluss-hand-authored -->
