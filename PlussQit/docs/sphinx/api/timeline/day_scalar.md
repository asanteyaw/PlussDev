<!-- pluss-hand-authored -->

# Timeline::day_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::day_scalar() const
```

## Read One Day Of The Month

`day_scalar()` returns the local day-of-month field for one rank-zero Timeline.
The valid range is `1..31`, constrained by the selected month and year. It does
not mean elapsed days since an epoch, ordinal day within a year, or weekday;
use `epoch_days()`, `yearday_scalar()`, or `weekday_scalar()` for those distinct
questions.

The scalar form avoids constructing a public `Series` result and fits directly
into ordinary C++ branching. [`day()`](day.md) is the shape-preserving form for
arrays. Both read the same timezone-aware civil field.

## Example: End Of February

```cpp
const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << "day=" << leap_day.day_scalar() << '\n';
```

### Output

```text
day=29
```

## Example: A Timezone Changes The Local Date

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:00Z");
const auto edmonton = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.day_scalar() << '\n'
          << "Edmonton=" << edmonton.day_scalar() << '\n';
```

### Output

```text
UTC=1
Edmonton=31
```

## Example: Absent And Non-Scalar Inputs

```cpp
std::cout << "absent=" << pluss::timeline::make("").day_scalar() << '\n';

try {
  static_cast<void>(
      pluss::timeline::make({"2024-02-29T23:45:00Z"}).day_scalar());
  std::cout << "accepted\n";
} catch (const std::exception&) {
  std::cout << "one-element vector rejected\n";
}
```

### Output

```text
absent=-1
one-element vector rejected
```

## Contract Summary

- The answer is a one-based local day of month.
- Rank-zero input is required; `{1}` shape is not scalar shape.
- `NaT` produces `-1`, outside the valid calendar range.
- The timezone is consulted, but the source Timeline is not changed.
- No rounding occurs and the source's subsecond precision remains available.

## See Also

- [`day()`](day.md): extract every element while preserving shape.
- [`yearday_scalar()`](yearday_scalar.md): ordinal day within the year.
- [`weekday_scalar()`](weekday_scalar.md): Monday-zero weekday.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html"><span class="label">Previous</span><span class="title">day</span></a>
  <a class="next-link" href="dayofyear.html"><span class="label">Next</span><span class="title">dayofyear</span></a>
</div>
