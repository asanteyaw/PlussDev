<!-- pluss-hand-authored -->

# Timeline::add_hours

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_hours(int hours) const
```

## Add Fixed Elapsed Hours

`add_hours()` advances each defined instant by `hours * 3,600,000,000,000`
nanoseconds. It is the concise scalar form for elapsed-hour arithmetic: session
lengths, processing delays, countdowns, and any rule where an hour means exactly
3,600 seconds.

The calculation occurs on the epoch axis. A timezone label controls how the
result is displayed, but it does not change the elapsed amount. This makes
`add_hours(24)` deliberately different from [`add_days(1)`](add_days.md) near
an offset transition.

## Across An Offset Change

Example 1. Twenty-four elapsed hours cross the spring transition. The UTC
distance stays 24 hours, so the displayed local clock advances by 25 hours.

```cpp
const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");
std::cout << pluss::note::print(pluss::timeline::format(
    spring.add_hours(24), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-10T04:30:00.000000000-06:00"
```

Example 2. A smaller elapsed shift passes directly over the nonexistent local
hour without requiring a gap-resolution policy.

```cpp
const auto before_gap = pluss::timeline::make("2024-03-10T08:30:00Z")
                            .tz_convert("America/Edmonton");
std::cout << pluss::note::print(pluss::timeline::format(
    before_gap.add_hours(2), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-10T04:30:00.000000000-06:00"
```

## Signed Counts And Precision

Positive values move later on the epoch axis, negative values move earlier,
and zero preserves every instant. Existing nanoseconds are retained exactly.
The method allocates a result and leaves its receiver unchanged.

Example 3. A negative shift crosses a date boundary without truncating the
fractional second.

```cpp
const auto source = pluss::timeline::make(
    "2025-01-01T01:15:00.123456789Z");
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_hours(-3), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T22:15:00.123456789+00:00"
```

## Shape, Strides, And Absence

The integer count is applied elementwise to any timeline rank. Shape and
timezone are retained; `NaT` propagates; empty shapes stay empty. Borrowed
non-contiguous values are read by logical stride, and the output is an owning
contiguous result.

Example 4. A stride-two view shifts only its two logical timestamps.

```cpp
std::array<std::int64_t, 4> backing{
    1'704'067'200'123'456'789LL, 111LL,
    1'709'164'800'000'000'004LL, 222LL};
const std::array<std::int64_t, 1> shape{2};
const std::array<std::int64_t, 1> strides{2};
const auto strided = pluss::Timeline::from_buffer(
    backing.data(), shape, strides, {}, "UTC");
std::cout << pluss::note::print(pluss::timeline::format(
    strided.add_hours(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-01T01:00:00.123456789+00:00", "2024-02-29T01:00:00.000000004+00:00"]
```

## Range Validation

Two bounds are checked: scaling the `int` count into nanoseconds must fit
signed 64-bit storage, and adding that offset to each defined epoch tick must
also fit. Either failure throws before a wrapped timestamp can be produced.

Example 5. An hour count whose nanosecond representation cannot fit is
rejected.

```cpp
try {
  (void)pluss::timeline::make("1970-01-01T00:00:00Z")
      .add_hours(2'147'483'647);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_days`](add_days.md): preserve the local clock across civil days.
- [`Timeline::add_minutes`](add_minutes.md): fixed elapsed minutes.
- [`Timeline::add_duration`](add_duration.md): add scalar or shaped durations.

