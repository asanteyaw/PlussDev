<!-- pluss-hand-authored -->

# Timeline::add_months

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_months(int months) const
```

## Shift On The Calendar Month Axis

`add_months()` moves each defined instant by a signed number of calendar
months. It is the direct expression of rules such as *one month after billing
date* or *the corresponding date in the previous quarter*. Month lengths are
not interchangeable elapsed durations, so this method does not approximate a
month as 30 days.

The implementation converts each instant to local calendar fields in `tz()`,
adds `months` to the year/month pair, clamps the day when necessary, and then
resolves the preserved clock fields back to an instant. Hours, minutes,
seconds, and all nine fractional digits survive the operation.

## Destination-Day Clamping

When the original day number does not exist in the destination month, the day
is clamped to that month's final day. January 31 plus one month therefore
becomes February 28 or 29; March 31 plus one month becomes April 30. This rule
is applied per element because different source dates can land in different
destination months.

Example 1. Positive month movement demonstrates ordinary preservation,
February clamping, a 30-day destination, and a year boundary in one 2-D call.

```cpp
const auto calendar = pluss::timeline::make({
    {"2024-02-29T08:15:30.123456789Z", "2023-01-31T23:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-12-31T23:59:59.999999999Z"},
});
std::cout << pluss::note::print(pluss::timeline::format(
    calendar.add_months(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-03-29T08:15:30.123456789+00:00", "2023-02-28T23:00:00.000000000+00:00"], 
  ["2024-04-30T12:00:00.000000000+00:00", "2025-01-31T23:59:59.999999999+00:00"]
]
```

## Signed Counts And Large Month Steps

Positive counts move forward, negative counts move backward, and zero returns
the same instants in a new value. Counts larger than twelve are valid: the
year and month are normalized together rather than processing one month at a
time.

Example 2. A negative month traverses both a leap-year February and a year
boundary.

```cpp
std::cout << pluss::note::print(pluss::timeline::format(
    calendar.add_months(-1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-29T08:15:30.123456789+00:00", "2022-12-31T23:00:00.000000000+00:00"], 
  ["2024-02-29T12:00:00.000000000+00:00", "2024-11-30T23:59:59.999999999+00:00"]
]
```

Example 3. Fifteen months is one normalized calculation, and clamping is based
on the final destination month.

```cpp
const auto source = pluss::timeline::make("2024-11-30T06:00:00Z");
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_months(15), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2026-02-28T06:00:00.000000000+00:00"
```

## Local-Time Resolution

For a timezone-bearing timeline, month arithmetic preserves the displayed
local clock, not a fixed UTC offset. The destination's timezone rules determine
the resulting instant and displayed offset. If the preserved clock fields land
inside a daylight-saving gap, the method rejects the operation rather than
silently choosing a different wall time. An ambiguous repeated clock is
resolved using the first occurrence.

Example 4. The local clock remains `03:30` even when the destination month uses
a different UTC offset.

```cpp
const auto local = pluss::timeline::make("2024-02-10T10:30:00Z")
                       .tz_convert("America/Edmonton");
std::cout << pluss::note::print(pluss::timeline::format(
    local.add_months(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-10T03:30:00.000000000-06:00"
```

## Shape, Absence, And Ownership

The single count applies elementwise to scalars and every array rank. Shape and
timezone metadata are retained. `NaT` propagates without calendar resolution;
empty shapes remain empty. The result owns its output storage and the receiver
is unchanged. Logical strides are honored when the receiver borrows a
non-contiguous buffer.

Example 5. Defined and absent elements follow separate paths without changing
the vector shape.

```cpp
const auto values = pluss::timeline::from_epoch(
    {1'704'067'200'123'456'789LL, pluss::Timeline::kNaT}, "UTC");
std::cout << pluss::note::print(pluss::timeline::format(
    values.add_months(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-02-01T00:00:00.123456789+00:00", "NaT"]
```

## Failure Conditions

The normalized destination year must stay in the supported calendar and the
resolved instant must fit signed 64-bit epoch nanoseconds. A nonexistent local
destination is also invalid. Validation is fail-fast: no partial result is
returned.

Example 6. A destination inside the spring clock gap is rejected.

```cpp
try {
  const auto local = pluss::timeline::make("2024-02-10T09:30:00Z")
                         .tz_convert("America/Edmonton");
  (void)local.add_months(1);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_years`](add_years.md): express annual calendar movement directly.
- [`Timeline::add_days`](add_days.md): move by local civil days.
- [`Timeline::add_duration`](add_duration.md): add fixed elapsed time.

