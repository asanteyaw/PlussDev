# Timeline::tz_convert

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::tz_convert(std::string_view tz_name) -> Timeline
```

## What It Does

Changes the timezone view of the same underlying instants.

This is the correct surface when the timestamps already represent real moments
in time and you want to view or print them in a different timezone.

## Parameters

**tz_name**

The destination timezone name.

Examples include:

- `"UTC"`
- `"America/New_York"`
- `"Europe/London"`

The timezone must exist in the Pluss timezone registry.

## Returns

`Timeline`
    A timeline with the same instants and new timezone interpretation.

## Examples

Example 1. Convert localized New York wall-clock times into UTC.

```cpp
auto out = ny.tz_convert("UTC");
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 13:00:00, 2024-11-01 14:15:30 ]
```

## Notes

- `tz_convert()` changes the timezone view of already-defined instants.
- Use [`tz_localize`](tz_localize.md) when you need to attach timezone meaning
  to local wall-clock timestamps instead of merely re-viewing existing instants.

## See Also

- [`Timeline::tz_localize`](tz_localize.md)
- [`Timeline::to_utc`](to_utc.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="tz_localize.html">
    <span class="label">Next</span>
    <span class="title">tz_localize</span>
  </a>
</div>
