<!-- pluss-hand-authored -->

# Timeline::yearday_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::yearday_scalar() const
```

## Read One Ordinal Day

`yearday_scalar()` returns the local date's one-based ordinal position within
its civil year. January 1 is `1`; December 31 is `365` in a common year and
`366` in a leap year. This makes it useful for annual schedules and seasonal
coordinates where month lengths should already be folded into one monotonic
field.

It is not an elapsed-day count. The value resets at each local New Year and is
computed from the Timeline's timezone. Use [`days_between()`](days_between.md)
to measure two instants, or [`epoch_days()`](epoch_days.md) for an epoch-based
projection.

The receiver must be rank zero. [`dayofyear()`](dayofyear.md) provides the same
calendar field as a shape-preserving `Series` for array-shaped Timelines.

## Example: Leap Day Has Ordinal 60

```cpp
const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << leap_day.yearday_scalar() << '\n';
```

### Output

```text
60
```

February 29 follows 31 January days and 28 earlier February days.

## Example: Leap-Year Boundaries

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-12-31T23:59:59Z");
std::cout << "start=" << start.yearday_scalar()
          << ", end=" << end.yearday_scalar() << '\n';
```

### Output

```text
start=1, end=366
```

## Example: Timezone And Absence

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:00Z");
const auto edmonton = utc.tz_convert("America/Edmonton");
const auto nat = pluss::timeline::make("");

std::cout << "UTC=" << utc.yearday_scalar()
          << ", Edmonton=" << edmonton.yearday_scalar()
          << ", absent=" << nat.yearday_scalar() << '\n';
```

### Output

```text
UTC=1, Edmonton=366, absent=-1
```

The Edmonton view is still on the final local day of leap year 2024.

## Gotchas And Failure Modes

- The field is one-based and resets at local New Year.
- Leap years change all values after February 28 and permit result `366`.
- `NaT` returns `-1`; rank-one and higher input throws.
- Timezone conversion can change both the year and ordinal day.
- The source instant, shape, timezone, and precision are not mutated.

## See Also

- [`dayofyear()`](dayofyear.md): array-shaped ordinal extraction.
- [`year_scalar()`](year_scalar.md): enclosing civil year.
- [`leap_scalar()`](is_leap_year.md): scalar leap-year classification.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year_scalar.html"><span class="label">Previous</span><span class="title">year_scalar</span></a>
  <a class="next-link" href="years_between.html"><span class="label">Next</span><span class="title">years_between</span></a>
</div>
