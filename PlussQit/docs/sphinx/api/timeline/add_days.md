<!-- pluss-hand-authored -->

# Timeline::add_days

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_days(int days) const
```

## Move By Civil Days

`add_days()` changes the local calendar date while preserving the local clock
fields. It is for rules stated as *tomorrow at the same local time*, *three
calendar days earlier*, or *the corresponding wall-clock appointment next
day*.

A civil day and 24 elapsed hours are not always the same interval. Timezone
offset changes can make the instant-to-instant distance 23 or 25 hours even
though the displayed date advances by exactly one. This distinction is why
`add_days(1)` is intentionally different from `add_hours(24)` and from adding
a fixed one-day `Duration`.

## Daylight-Saving Transitions

The method reads each instant in the timeline's timezone, moves its civil date,
and resolves the same hour, minute, second, and nanosecond on that date. The
destination's UTC offset is then applied.

Example 1. Across the spring offset change, the local clock remains `03:30`.
The same source plus 24 elapsed hours lands at `04:30`.

```cpp
const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");
std::cout << pluss::note::print(pluss::timeline::format(
    spring, "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    spring.add_days(1), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    spring.add_hours(24), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-09T03:30:00.000000000-07:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-10T03:30:00.000000000-06:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-10T04:30:00.000000000-06:00"
```

Example 2. Inspecting epoch nanoseconds proves that the calendar-day result is
23 elapsed hours after the source in this transition.

```cpp
std::cout << pluss::series::print(
    spring.add_days(1).epoch_nanos() - spring.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[82800000000000]
```

At the autumn transition the same rule can span 25 elapsed hours. The method's
guarantee is the local date and clock, not a constant epoch-nanosecond delta.

## Signed Counts, Precision, And Mutation

`days` is signed: positive moves forward, negative moves backward, and zero
preserves the instant. All subsecond digits are retained. The method is
out-of-place and applies one scalar count to every logical element; it does not
mutate the source or broadcast a second operand.

Example 3. Positive, negative, and zero counts preserve nanoseconds.

```cpp
const auto precise = pluss::timeline::make(
    "2024-12-31T23:59:30.123456789Z");
std::cout << pluss::note::print(pluss::timeline::format(
    precise.add_days(1), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    precise.add_days(-1), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    precise.add_days(0), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2025-01-01T23:59:30.123456789+00:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-30T23:59:30.123456789+00:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:59:30.123456789+00:00"
```

## Shape, Timezone, And Absence

Scalar, vector, matrix, and higher-rank timelines are accepted. The returned
timeline has the same shape and timezone label. Logical strides are respected,
`NaT` remains `NaT`, and zero-sized dimensions are preserved.

Example 4. An absent instant propagates while the defined value moves three
civil days.

```cpp
const auto values = pluss::timeline::from_epoch(
    {1'704'067'200'123'456'789LL, pluss::Timeline::kNaT}, "UTC");
std::cout << pluss::note::print(pluss::timeline::format(
    values.add_days(3), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-04T00:00:00.123456789+00:00", "NaT"]
```

Example 5. Empty rank is metadata, not an error.

```cpp
const auto empty = pluss::timeline::epoch({0, 2}).add_days(1);
std::cout << pluss::note::print(pluss::timeline::format(
    empty, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[0, 2], dim=2, numel=0, encoding=utf8)
[]
```

## Nonexistent And Ambiguous Local Clocks

If the preserved wall clock does not exist on the destination date, the method
throws instead of silently moving the timestamp. A repeated clock at an autumn
transition resolves to its first occurrence. Use fixed elapsed arithmetic when
you want to cross transitions without requesting a particular local clock.

Example 6. `02:30` exists on March 9 but not on the spring-transition date in
this timezone, so adding one civil day is rejected.

```cpp
try {
  const auto before = pluss::timeline::make("2024-03-09T09:30:00Z")
                          .tz_convert("America/Edmonton");
  (void)before.add_days(1);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_hours`](add_hours.md): add fixed elapsed hours.
- [`Timeline::add_duration`](add_duration.md): broadcast and add a `Duration`.
- [`Timeline::add_months`](add_months.md): calendar movement with month-end rules.

