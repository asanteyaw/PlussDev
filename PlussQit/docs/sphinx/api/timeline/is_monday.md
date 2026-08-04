# Timeline::is_monday

[Back to Timeline API Reference](../../timeline_api_reference)

## Representative Signatures

```cpp
Timeline::is_monday() -> Series
Timeline::is_tuesday() -> Series
Timeline::is_wednesday() -> Series
Timeline::is_thursday() -> Series
Timeline::is_friday() -> Series
Timeline::is_saturday() -> Series
Timeline::is_sunday() -> Series
```

## What It Does

Returns weekday-specific boolean masks.

Use this family when your logic cares about one named weekday rather than the
coarser `is_weekend()` or `is_business_day()` predicates.

## Returns

`Series`
    A boolean series for the requested weekday test.

## Examples

Example 1. Identify Monday rows and compare them with the weekend masks.

### Output

`calendar.is_monday()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, true, false, false, false, true]
```

`calendar.is_friday()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, true, false, false, false, false, false, false]
```

`calendar.is_saturday()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, false, true, false]
```

`calendar.is_sunday()`:

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, true, false, false, false, false, false]
```

## Aliases

Representative aliases:

- `is_mon()`, `mon_is()`
  Alias `is_monday()`.
- `is_tue()`
  Alias `is_tuesday()`.
- `is_wed()`
  Alias `is_wednesday()`.
- `is_thu()`
  Alias `is_thursday()`.
- `is_fri()`
  Alias `is_friday()`.
- `is_sat()`
  Alias `is_saturday()`.
- `is_sun()`
  Alias `is_sunday()`.

## Notes

- These are the specific-day refinement of the broader calendar predicate
  family.

## See Also

- [`Timeline::is_leap_year`](is_leap_year.md)
- [`Timeline::weekday_name`](weekday_name.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_leap_year.html">
    <span class="label">Previous</span>
    <span class="title">is_leap_year</span>
  </a>
  <a class="next-link" href="date_range.html">
    <span class="label">Next</span>
    <span class="title">date_range</span>
  </a>
</div>
