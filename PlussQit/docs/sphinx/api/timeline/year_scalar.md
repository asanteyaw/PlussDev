<!-- pluss-hand-authored -->

# Timeline::year_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::year_scalar() const
```

## Read One Civil Year As A C++ Value

`year_scalar()` extracts the year displayed for one scalar instant in the
Timeline's current timezone. It returns an ordinary `int`, which makes it the
direct choice for control flow, scalar validation, and C++ APIs that do not
accept a `Series`.

The receiver must be rank zero. A one-element vector still has rank one and is
rejected; element count does not erase shape. Use [`year()`](year.md) for any
array-shaped Timeline. That method preserves shape and returns a `Series`,
whereas `year_scalar()` deliberately proves that exactly one scalar instant is
being consumed.

The result is a civil field, not a property of the stored UTC tick alone.
Changing `tz()` can move an instant across midnight or New Year and therefore
change the answer without changing the instant.

## Example: Leap-Year Date

```cpp
const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << leap_day.year_scalar() << '\n';
```

### Output

```text
2024
```

## Example: The Same Instant In Two Civil Years

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:00Z");
const auto edmonton = utc.tz_convert("America/Edmonton");

std::cout << "UTC=" << utc.year_scalar() << '\n'
          << "Edmonton=" << edmonton.year_scalar() << '\n';
```

### Output

```text
UTC=2025
Edmonton=2024
```

This is not truncation or reparsing. Both values identify the same instant;
their active civil calendars place it on opposite sides of New Year.

## Example: Absent Instant And Rank Failure

```cpp
const auto nat = pluss::timeline::make("");
std::cout << "absent=" << nat.year_scalar() << '\n';

const auto one_vector = pluss::timeline::make({"2024-02-29T23:45:00Z"});
try {
  static_cast<void>(one_vector.year_scalar());
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

`-1` is the absent-field sentinel. Test [`isnat_scalar()`](isnat_scalar.md)
before extraction when absence must not be confused with an integer result.

## Contract Summary

- The receiver must be rank zero, including for `NaT`.
- The active timezone determines the civil year.
- The method allocates an internal scalar projection but returns an `int`.
- It does not mutate the Timeline or discard nanosecond precision from it.
- `NaT` returns `-1`; non-scalar input throws.

## See Also

- [`year()`](year.md): shape-preserving year extraction.
- [`week()`](week.md): ISO week number, which can belong to an adjacent week-year.
- [`tz_convert()`](tz_convert.md): change the civil view without changing instants.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html"><span class="label">Previous</span><span class="title">year</span></a>
  <a class="next-link" href="yearday_scalar.html"><span class="label">Next</span><span class="title">yearday_scalar</span></a>
</div>
