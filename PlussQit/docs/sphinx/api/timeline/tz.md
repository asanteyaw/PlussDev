<!-- pluss-hand-authored -->

# Timeline::tz

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::string_view Timeline::tz() const noexcept
```

## What It Tells You

`tz()` returns the timezone used to interpret and display the timeline's
calendar fields. The stored machine representation remains UTC epoch
nanoseconds; timezone metadata determines which local year, month, day, hour,
and offset those instants present.

That separation lets [`tz_convert()`](tz_convert.md) change the presentation
zone without changing the represented instant. Two timelines can therefore
have different `tz()` results and identical epoch values.

The returned `std::string_view` borrows from the timeline. Copy it into a
`std::string` if the name must be retained after the timeline is destroyed or
reassigned.

## Returns

A borrowed timezone name such as `"UTC"` or `"America/New_York"`.

## Examples

Example 1. Text ending in `Z` constructs an instant presented in UTC.

```cpp
auto instant = pluss::timeline::make("2024-02-29T12:30:00Z");
std::cout << "timezone=" << instant.tz() << "\n";
```

### Output

```text
timezone=UTC
```

Example 2. Converting presentation zones preserves the machine instant.

```cpp
auto utc = pluss::timeline::make("2024-02-29T12:30:00Z");
auto local = utc.tz_convert("America/New_York");

std::cout << std::boolalpha
          << "UTC: tz=" << utc.tz()
          << ", epoch=" << utc.to_epoch_nanos_vector().front() << "\n"
          << "New York: tz=" << local.tz()
          << ", epoch=" << local.to_epoch_nanos_vector().front() << "\n"
          << "same instant: "
          << (utc.to_epoch_nanos_vector() == local.to_epoch_nanos_vector())
          << "\n";
```

### Output

```text
UTC: tz=UTC, epoch=1709209800000000000
New York: tz=America/New_York, epoch=1709209800000000000
same instant: true
```

Example 3. The local rendering changes even though the epoch does not.

```cpp
auto utc = pluss::timeline::make("2024-02-29T12:30:00Z");
auto local = utc.tz_convert("America/New_York");
std::cout << utc.isoformat() << "\n" << local.isoformat() << "\n";
```

### Output

```text
2024-02-29T12:30:00.000000000+00:00
2024-02-29T07:30:00.000000000-05:00
```

## Gotchas And Failure Modes

- `tz()` returns metadata, not a numeric UTC offset. Offsets vary by instant
  in zones with historical or seasonal transitions.
- `tz_convert()` preserves the instant. `tz_localize()` assigns a timezone
  interpretation to wall-clock values and has ambiguity/nonexistence rules.
- The `string_view` is borrowed; do not retain it after its timeline's
  lifetime.
- An unknown timezone name is rejected by operations that load timezone rules.

## Summary

The epoch tick answers *when*; `tz()` answers *where that instant is being
read on a wall clock*.

## See Also

- [`Timeline::tz_convert`](tz_convert.md)
- [`Timeline::tz_localize`](tz_localize.md)
- [`Timeline::to_utc`](to_utc.md)
- [`Timeline::to_epoch_nanos_vector`](to_epoch_nanos_vector.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_scalar.html"><span class="label">Previous</span><span class="title">is_scalar</span></a>
  <a class="next-link" href="host.html"><span class="label">Next</span><span class="title">host</span></a>
</div>
