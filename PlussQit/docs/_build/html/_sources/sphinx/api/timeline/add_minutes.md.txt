<!-- pluss-hand-authored -->

# Timeline::add_minutes

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_minutes(int minutes) const
```

## Add Fixed Elapsed Minutes

`add_minutes()` adds a scalar number of exact 60-second intervals to every
defined instant. It is designed for delays, timeouts, lead times, and clock
adjustments naturally expressed as an integral minute count.

The operation adds `minutes * 60,000,000,000` nanoseconds on the epoch axis.
It does not round the input to a minute and does not clear seconds or fractional
seconds. Use [`floor`](floor.md) when quantization is the goal.

Example 1. Adding two minutes crosses both midnight and the year boundary while
preserving seconds and all nine fractional digits.

```cpp
const auto source = pluss::timeline::make(
    "2024-12-31T23:59:30.123456789Z");
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_minutes(2), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2025-01-01T00:01:30.123456789+00:00"
```

## Direction, Zero, And Mutation

Positive values move later, negative values move earlier, and zero leaves each
instant unchanged. A new timeline is returned in every case; the source is not
modified.

Example 2. Negative and zero counts have direct signed meanings.

```cpp
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_minutes(-2), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_minutes(0), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:57:30.123456789+00:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:59:30.123456789+00:00"
```

## Shape, Timezone, And Absence

The scalar count applies to every logical element without changing rank or
shape. The timezone label is retained and may cause a transition-crossing
result to display with a different UTC offset. `NaT` stays `NaT`; empty inputs
remain empty; strided borrowed storage is read correctly.

Example 3. A matrix remains a matrix and preserves absence.

```cpp
const auto values = pluss::timeline::make({
    {"2024-01-01T09:00:00Z", "2024-01-01T09:30:15.000000004Z"},
    {"", "2024-01-01T23:59:45Z"},
});
std::cout << pluss::note::print(pluss::timeline::format(
    values.add_minutes(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-01T09:01:00.000000000+00:00", "2024-01-01T09:31:15.000000004+00:00"], 
  ["NaT", "2024-01-02T00:00:45.000000000+00:00"]
]
```

## Range Validation

Both the unit-to-nanosecond conversion and each epoch addition are checked.
The method throws if either would exceed signed 64-bit epoch storage; it never
wraps into an unrelated date.

Example 4. An unrepresentable scaled count is rejected.

```cpp
try {
  (void)pluss::timeline::make("1970-01-01T00:00:00Z")
      .add_minutes(2'147'483'647);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_hours`](add_hours.md): fixed elapsed hours.
- [`Timeline::add_seconds`](add_seconds.md): fixed elapsed seconds.
- [`Timeline::add_duration`](add_duration.md): shaped and broadcast duration arithmetic.

