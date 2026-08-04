# Timeline::is_leap_year

[Back to Timeline API Reference](../../timeline_api_reference)

## Representative Signatures

```cpp
Timeline::is_leap_year() -> Series
Timeline::is_month_start() -> Series
Timeline::is_month_end() -> Series
Timeline::is_quarter_start() -> Series
Timeline::is_quarter_end() -> Series
Timeline::is_year_start() -> Series
Timeline::is_year_end() -> Series
Timeline::is_weekend() -> Series
Timeline::is_business_day() -> Series
```

## What It Does

This page documents the calendar predicate family: methods that answer yes/no
questions about each timestamp’s calendar position.

## Returns

`Series`
    A boolean series marking which entries satisfy the predicate.

## Examples

Example 1. Check leap-year membership and calendar boundaries.

### Output

`calendar.is_leap_year()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, true, true, true, true, false, true, true]
```

`calendar.is_month_start()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, true, false, true, false, true, false, false]
```

`calendar.is_month_end()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, false, true, false, true, false, false, false]
```

`calendar.is_year_end()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, true, false, false, false]
```

Example 2. Check business-day versus weekend classification.

### Output

`calendar.is_weekend()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, true, false, false, false, true, false]
```

`calendar.is_business_day()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, true, false, true, true, true, false, true]
```

## Aliases

Representative alias groups include:

- `leap_year()`, `is_leap()`
  Alias `is_leap_year()`.
- `is_month_begin()`
  Alias `is_month_start()`.
- `is_month_finish()`
  Alias `is_month_end()`.
- `is_qs()`
  Alias `is_quarter_start()`.
- `is_qe()`
  Alias `is_quarter_end()`.
- `is_year_begin()`
  Alias `is_year_start()`.
- `is_year_finish()`
  Alias `is_year_end()`.
- `weekend()`
  Alias `is_weekend()`.
- `is_bday()`, `business_day()`
  Alias `is_business_day()`.

## Notes

- These predicates read calendar structure in the timeline’s current timezone.
- They are especially useful before `where(...)`, filtering, or time-aware
  aggregation.

## See Also

- [`Timeline::year`](year.md)
- [`Timeline::is_monday`](is_monday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
  <a class="next-link" href="is_monday.html">
    <span class="label">Next</span>
    <span class="title">is_monday</span>
  </a>
</div>
