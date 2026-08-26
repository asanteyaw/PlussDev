# Timeline::year

[Back to Timeline API Reference](../../timeline_api_reference)

## Representative Signatures

```cpp
Timeline::year() -> Series
Timeline::month() -> Series
Timeline::day() -> Series
Timeline::hour() -> Series
Timeline::minute() -> Series
Timeline::second() -> Series
Timeline::nanosecond() -> Series
Timeline::microsecond() -> Series
Timeline::millisecond() -> Series
Timeline::weekday() -> Series
Timeline::dayofyear() -> Series
Timeline::quarter() -> Series
Timeline::week() -> Series
```

## What It Does

This page is the canonical reference for the calendar component extractor
family.

These methods project each timestamp into one numeric calendar field.

## Returns

`Series`
    A numeric series for the requested calendar component.

## Examples

Example 1. Extract coarse calendar components.

```cpp
auto years = base.year();
auto months = base.month();
auto days = base.day();
std::cout << pluss::series::print(years) << "\n";
std::cout << pluss::series::print(months) << "\n";
std::cout << pluss::series::print(days) << "\n";
```

### Output

`base.year()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[2024, 2024, 2024, 2025]
```

`base.month()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[11, 11, 12, 1]
```

`base.day()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 31, 1]
```

Example 2. Extract time-of-day components.

```cpp
auto hours = base.hour();
auto minutes = base.minute();
auto seconds = base.second();
std::cout << pluss::series::print(hours) << "\n";
std::cout << pluss::series::print(minutes) << "\n";
std::cout << pluss::series::print(seconds) << "\n";
```

### Output

`base.hour()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[9, 10, 23, 0]
```

`base.minute()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[0, 15, 59, 0]
```

`base.second()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[0, 30, 59, 0]
```

Example 3. Extract finer and calendar-indexing components.

```cpp
auto nanos = base.nanosecond();
auto weekdays = base.weekday();
auto yeardays = base.dayofyear();
auto quarters = base.quarter();
auto weeks = base.week();
std::cout << pluss::series::print(nanos) << "\n";
std::cout << pluss::series::print(weekdays) << "\n";
std::cout << pluss::series::print(yeardays) << "\n";
std::cout << pluss::series::print(quarters) << "\n";
std::cout << pluss::series::print(weeks) << "\n";
```

### Output

`base.nanosecond()`:

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 0, 0]
```

`base.weekday()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[4, 4, 1, 2]
```

`base.dayofyear()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[306, 306, 366, 1]
```

`base.quarter()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[4, 4, 4, 1]
```

`base.week()`:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[44, 44, 1, 1]
```

## Aliases

Many short aliases route to the same implementations. Representative examples:

- `y`, `yr`, `years`
  Alias `year()`.
- `mon`, `mo`
  Alias `month()`.
- `d`, `dom`
  Alias `day()`.
- `h`, `hr`
  Alias `hour()`.
- `m`
  Alias `minute()`.
- `sec`, `s`
  Alias `second()`.
- `dow`, `dayofweek`
  Alias `weekday()`.
- `doy`
  Alias `dayofyear()`.
- `q`, `qtr`
  Alias `quarter()`.
- `weekofyear`, `woy`
  Alias `week()`.

## Notes

- All these methods read calendar fields in the timeline’s current timezone.
- When you only need text labels instead of numeric fields, use
  [`weekday_name`](weekday_name.md), [`month_name`](month_name.md), or the
  `to_note_*` helpers.

## See Also

- [`Timeline::is_leap_year`](is_leap_year.md)
- [`Timeline::is_monday`](is_monday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="searchsorted_left.html">
    <span class="label">Previous</span>
    <span class="title">searchsorted_left</span>
  </a>
  <a class="next-link" href="is_leap_year.html">
    <span class="label">Next</span>
    <span class="title">is_leap_year</span>
  </a>
</div>
