<!-- pluss-hand-authored -->

# Timeline::second_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::second_scalar() const
```

## Read One Whole-Second Clock Field

`second_scalar()` returns the integral second displayed within the scalar
instant's local minute. Its range is `0..59`. Fractional nanoseconds are not
rounded into this result: an instant at `45.999999999` still has second `45`.
The original Timeline retains the full fraction.

Use [`second()`](second.md) when operating on an array. The scalar method
returns an `int` and enforces rank zero, making accidental consumption of an
array a checked error rather than an implicit first-element operation.

## Example: Read A Clock Second

```cpp
const auto t = pluss::timeline::make("2025-01-01T01:30:45Z");
std::cout << t.second_scalar() << '\n';
```

### Output

```text
45
```

## Example: Fractional Precision Is Not Rounded

```cpp
const auto t = pluss::timeline::make("2025-01-01T01:30:45.999999999Z");
std::cout << "second=" << t.second_scalar()
          << ", nanos=" << t.nanosecond().item<std::int32_t>() << '\n';
```

### Output

```text
second=45, nanos=999999999
```

## Example: Timezone And Absence

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:45Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.second_scalar()
          << ", Edmonton=" << local.second_scalar()
          << ", absent=" << pluss::timeline::make("").second_scalar() << '\n';
```

### Output

```text
UTC=45, Edmonton=45, absent=-1
```

Timezone offsets normally preserve the second component, but the extraction
still occurs after applying the active timezone's civil view.

## Contract Summary

- Output is `0..59`; leap-second value `60` is not represented.
- `NaT` returns `-1`; non-scalar shape throws.
- Fractions are truncated from the returned field, not from the receiver.
- The operation is read-only.

## See Also

- [`second()`](second.md): shape-preserving extraction.
- [`nanosecond()`](nanosecond.md): fraction within the second.
- [`seconds_between()`](seconds_between.md): elapsed whole-second distance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html"><span class="label">Previous</span><span class="title">second</span></a>
  <a class="next-link" href="seconds_between.html"><span class="label">Next</span><span class="title">seconds_between</span></a>
</div>
