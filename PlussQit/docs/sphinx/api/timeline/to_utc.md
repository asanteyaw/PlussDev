# Timeline::to_utc

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::to_utc() -> Timeline
```

## What It Does

Converts a timezone-aware timeline into a UTC view.

This is the short explicit convenience form of `tz_convert("UTC")`.

## Returns

`Timeline`
    The same instants rendered in UTC.

## Examples

Example 1. Convert localized wall-clock values back into UTC.

```cpp
auto out = ny.to_utc();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 13:00:00, 2024-11-01 14:15:30 ]
```

## Notes

- Prefer `to_utc()` when UTC is the explicit target and readability matters.
- Prefer [`tz_convert`](tz_convert.md) when the target zone is not UTC.

## See Also

- [`Timeline::tz_convert`](tz_convert.md)
- [`Timeline::tz_localize`](tz_localize.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz_localize.html">
    <span class="label">Previous</span>
    <span class="title">tz_localize</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
