<!-- pluss-hand-authored -->

# Timeline::hour_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::hour_scalar() const
```

## Read One Local Clock Hour

`hour_scalar()` returns the hour displayed by one rank-zero Timeline, from `0`
through `23`. It uses a 24-hour clock and the Timeline's current timezone. It
does not return elapsed hours since midnight or since the epoch.

Use this native-value form for branching and ordinary C++ APIs. Use
[`hour()`](hour.md) for a vector, matrix, or higher-rank Timeline: `hour()`
preserves shape in a `Series`, while `hour_scalar()` rejects every non-scalar
shape, including a one-element vector.

## Example: Read The Hour

```cpp
const auto t = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << t.hour_scalar() << '\n';
```

### Output

```text
23
```

## Example: The Hour Belongs To The Active Timezone

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:45Z");
const auto edmonton = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.hour_scalar() << '\n'
          << "Edmonton=" << edmonton.hour_scalar() << '\n';
```

### Output

```text
UTC=1
Edmonton=18
```

Both Timelines identify the same instant. Conversion changes its civil-clock
view and crosses into the previous local date.

## Example: Absence And Rank Are Different Conditions

```cpp
std::cout << "absent=" << pluss::timeline::make("").hour_scalar() << '\n';
try {
  (void)pluss::timeline::make({"2025-01-01T01:30:45Z"}).hour_scalar();
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

- Valid hours are `0..23`; `NaT` returns `-1`.
- Rank zero is required regardless of element count.
- Timezone conversion may change the hour without changing the instant.
- The method is read-only and returns an ordinary `int`.

## See Also

- [`hour()`](hour.md): shape-preserving extraction.
- [`minute_scalar()`](minute_scalar.md): minute within the local hour.
- [`time()`](time.md): retain the complete local clock as a Timeline.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html"><span class="label">Previous</span><span class="title">hour</span></a>
  <a class="next-link" href="hours_between.html"><span class="label">Next</span><span class="title">hours_between</span></a>
</div>
