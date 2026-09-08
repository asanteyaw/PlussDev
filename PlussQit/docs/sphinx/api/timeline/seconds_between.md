# Timeline::seconds_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::seconds_between(const Timeline& other) const
```

## Count Complete Elapsed Seconds

`seconds_between()` returns the signed count of complete one-billion-nanosecond
units from the scalar receiver to scalar `other`. Fractional seconds are
truncated toward zero. The operation uses absolute epoch instants, not local
calendar fields, and therefore permits different timezone labels.

The 64-bit return type is part of the contract. Timeline's supported range can
span far more than the roughly 68 years representable by signed 32-bit seconds.
Internally, subtraction is widened before division so distant valid endpoints
cannot overflow merely because their raw nanosecond ticks have opposite signs.

## Example: Fractional Seconds Do Not Round

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");

std::cout << start.seconds_between(end) << '\n'
          << end.seconds_between(start) << '\n';
```

### Output

```text
93784
-93784
```

The trailing half-second is discarded in both directions. The reverse result
is not `-93785` because division truncates toward zero rather than flooring.

## Example: The Result Exceeds 32-Bit Range Safely

```cpp
const auto early = pluss::timeline::make("1700-01-01T00:00:00Z");
const auto late = pluss::timeline::make("2200-01-01T00:00:00Z");

std::cout << early.seconds_between(late) << '\n'
          << late.seconds_between(early) << '\n';
```

### Output

```text
15778454400
-15778454400
```

This interval also makes the direct nanosecond subtraction exceed signed
64-bit range. Widening before subtraction preserves the correct quotient.

## Example: Timezone Conversion Preserves The Instant

```cpp
const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");

std::cout << utc.seconds_between(local) << '\n';
```

### Output

```text
0
```

## Example: `NaT` And Arrays Are Not Scalar Distances

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");

for (const auto& other : {
         pluss::timeline::make(""),
         pluss::timeline::make({"2024-01-01", "2024-01-02"}),
     }) {
  try {
    (void)start.seconds_between(other);
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}
```

### Output

```text
rejected
rejected
```

The method performs constant work, allocates nothing, and never mutates either
operand. Use [`span()`](span.md) when the inputs are arrays or a subsecond unit
such as milliseconds, microseconds, or nanoseconds is required.

## See Also

- [`minutes_between`](minutes_between.md): count complete elapsed minutes.
- [`diff`](diff.md): obtain a shape-preserving `Duration` difference.
- [`span`](span.md): select other units and array behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second_scalar.html"><span class="label">Previous</span><span class="title">second_scalar</span></a>
  <a class="next-link" href="set_day.html"><span class="label">Next</span><span class="title">set_day</span></a>
</div>
<!-- pluss-hand-authored -->
