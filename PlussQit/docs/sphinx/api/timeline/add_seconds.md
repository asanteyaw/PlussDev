<!-- pluss-hand-authored -->

# Timeline::add_seconds

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_seconds(int seconds) const
```

## Add Integral Elapsed Seconds

`add_seconds()` adds a single signed count of exact one-billion-nanosecond
intervals to every defined instant. It is the direct method for integral-second
delays, clock corrections, and threshold windows when constructing a separate
`Duration` would add no information.

This is epoch-axis arithmetic. It does not replace the displayed second field,
round to a boundary, or preserve local wall-clock fields across timezone offset
changes. Existing fractional nanoseconds remain attached to the shifted
instant.

Example 1. Forty-five seconds crosses midnight without losing precision.

```cpp
const auto source = pluss::timeline::make(
    "2024-12-31T23:59:30.123456789Z");
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_seconds(45), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2025-01-01T00:00:15.123456789+00:00"
```

## Positive, Negative, And Zero Counts

Positive counts move toward later instants, negative counts subtract elapsed
seconds, and zero preserves the stored ticks. The returned timeline owns its
result; the receiver remains unchanged.

Example 2. A negative count crosses into the previous minute.

```cpp
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_seconds(-90), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:58:00.123456789+00:00"
```

Example 3. Printing both a zero-shift result and the source makes the no-op and
non-mutation contracts explicit.

```cpp
const auto shifted = source.add_seconds(45);
std::cout << pluss::note::print(pluss::timeline::format(
    source.add_seconds(0), "%FT%T.%f%z")) << '\n';
std::cout << pluss::note::print(pluss::timeline::format(
    source, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:59:30.123456789+00:00"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-12-31T23:59:30.123456789+00:00"
```

## Rank, Timezone, And Absence

The one integer argument applies elementwise to scalar and array timelines. No
broadcasting is involved. The output retains shape and timezone metadata,
follows logical strides, and propagates `NaT`. A zero-sized input returns the
same zero-sized shape.

Example 4. Defined and absent values remain distinguishable after the shift.

```cpp
const auto values = pluss::timeline::from_epoch(
    {1'704'067'200'123'456'789LL, pluss::Timeline::kNaT}, "UTC");
std::cout << pluss::note::print(pluss::timeline::format(
    values.add_seconds(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-01T00:00:01.123456789+00:00", "NaT"]
```

## Overflow Is An Error

The `int` second count itself always scales safely into signed 64-bit
nanoseconds, but the final addition can still exceed the epoch storage range
for an instant near an endpoint. That addition is checked for every defined
element. Overflow throws instead of wrapping.

Example 5. A near-maximum epoch tick cannot accept another second.

```cpp
try {
  (void)pluss::timeline::from_epoch(
      std::int64_t{9'223'372'036'854'775'000LL}).add_seconds(1);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`Timeline::add_minutes`](add_minutes.md): fixed elapsed minutes.
- [`Timeline::add_nanos`](add_nanos.md): raw nanosecond control with a 64-bit count.
- [`Timeline::set_second`](set_second.md): replace the local second field instead of shifting.

