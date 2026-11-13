# Timeline::tz_localize

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::tz_localize(std::string_view tz_name, int fold = 0, std::string_view nonexistent = "raise")
```

## What It Does

Attaches timezone meaning to local wall-clock timestamps and converts them into
real UTC-backed instants under that timezone.

This is not the same as `tz_convert()`:

- `tz_localize(...)`
  Says what timezone the stored wall-clock values should be interpreted in.
- `tz_convert(...)`
  Re-views already-defined instants in another timezone.

## Parameters

**tz_name**

The timezone whose wall-clock rules should be applied.

**fold**

Controls ambiguous repeated local times during fall-back transitions.

- `0`
  Prefer the earlier interpretation.
- `1`
  Prefer the later interpretation.

Most ordinary timestamps are not ambiguous, so this mainly matters around DST
fall-back hours.

**nonexistent**

Controls how nonexistent local wall-clock times should be handled.

- `"raise"`
  Treat the impossible local time as an error.

The current implementation exposes the policy parameter even when your concrete
example does not trigger it.

## Returns

`Timeline`
    A timezone-aware timeline.

## Examples

Example 1. Interpret stored wall-clock values as New York local time.

```cpp
auto out = local_wall.tz_localize("America/New_York");
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=America/New_York)
[ 2024-11-01 09:00:00, 2024-11-01 10:15:30 ]
```

## Notes

- `tz_localize()` changes the underlying instant because it reinterprets local
  wall-clock values.
- Use `fold` and `nonexistent` intentionally around DST boundary work.

## See Also

- [`Timeline::tz_convert`](tz_convert.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz_convert.html">
    <span class="label">Previous</span>
    <span class="title">tz_convert</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
