<!-- pluss-hand-authored -->

# Timeline::add_years

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_years(int years) const
```

## Shift By Calendar Years

`add_years()` moves every defined instant by an integer number of years on the
calendar displayed by the `Timeline`. It exists for rules stated in years:
anniversary dates, annual review points, and the same local appointment in a
later or earlier year.

This is calendar arithmetic, not multiplication by 365 days. The method first
reads each instant as local year, month, day, and clock fields in `tz()`, changes
the year, and resolves those fields back to an instant. Consequently it keeps
the local month, day, time of day, and nanosecond whenever that date exists.

Use `add_years(n)` when *same calendar position, n years away* is the rule. Use
[`add_duration`](add_duration.md) when the rule is a fixed elapsed interval.

## Leap-Day Rule

A leap-day anniversary may land in a year without February 29. In that one
case, `add_years()` clamps the day to the final valid day of the destination
month. Thus February 29 becomes February 28; it does not spill into March.
Other valid month/day pairs retain their day exactly.

Example 1. Shift a leap day and ordinary dates forward one year. The 2-D shape
and every subsecond field are retained.

```cpp
const auto calendar = pluss::timeline::make({
    {"2024-02-29T08:15:30.123456789Z", "2023-01-31T23:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-12-31T23:59:59.999999999Z"},
});

std::cout << pluss::note::print(pluss::timeline::format(
    calendar.add_years(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2025-02-28T08:15:30.123456789+00:00", "2024-01-31T23:00:00.000000000+00:00"], 
  ["2025-03-31T12:00:00.000000000+00:00", "2025-12-31T23:59:59.999999999+00:00"]
]
```

## Direction And Zero

`years` is signed. Positive values move toward later calendar years, negative
values move toward earlier years, and zero reconstructs the same local fields.
The operation is out of place: it returns a new `Timeline` and never rewrites
the receiver.

Example 2. A negative count applies the same leap-day clamp in the earlier
year.

```cpp
std::cout << pluss::note::print(pluss::timeline::format(
    calendar.add_years(-1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2023-02-28T08:15:30.123456789+00:00", "2022-01-31T23:00:00.000000000+00:00"], 
  ["2023-03-31T12:00:00.000000000+00:00", "2023-12-31T23:59:59.999999999+00:00"]
]
```

Example 3. A zero shift leaves the value unchanged; the separate source print
also demonstrates that a nonzero shift does not mutate its receiver.

```cpp
const auto source = pluss::timeline::make("2024-02-29T08:15:30.123456789Z");
const auto shifted = source.add_years(1);
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_years(0), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    source, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-02-29T08:15:30.123456789+00:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-02-29T08:15:30.123456789+00:00"
```

## Shape, Timezone, And Absent Instants

The scalar count applies elementwise to any rank. The output has the receiver's
shape and timezone label, including for non-contiguous borrowed storage.
Defined values are read through logical strides. `NaT` remains `NaT`, and an
empty input returns an empty output with the same shape.

Example 4. Leap-day clamping also works through a stride-two borrowed view; the
padding elements are not mistaken for timestamps.

```cpp
std::array<std::int64_t, 4> backing{
    1'704'067'200'123'456'789LL, 111LL,
    1'709'164'800'000'000'004LL, 222LL};
const std::array<std::int64_t, 1> shape{2};
const std::array<std::int64_t, 1> strides{2};
const auto strided = pluss::Timeline::from_buffer(
    backing.data(), shape, strides, {}, "UTC");

std::cout << pluss::note::print(pluss::timeline::format(
    strided.add_years(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2025-01-01T00:00:00.123456789+00:00", "2025-02-28T00:00:00.000000004+00:00"]
```

## Failure Conditions

The destination must be representable by the supported civil calendar and by
the signed 64-bit epoch-nanosecond storage. The method rejects an out-of-range
year and rejects a reconstructed local wall time that does not exist in the
timeline's timezone. It never returns a partially shifted result.

Example 5. Advancing beyond the calendar range is rejected.

```cpp
try {
  (void)pluss::timeline::make("9999-12-31T00:00:00Z").add_years(1);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_months`](add_months.md): shift by calendar months with month-end clamping.
- [`Timeline::add_days`](add_days.md): preserve local clock fields while changing the civil day.
- [`Timeline::add_duration`](add_duration.md): add a fixed elapsed `Duration`.

