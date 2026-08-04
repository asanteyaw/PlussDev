# Timeline::round

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::round(std::string_view unit) -> Timeline
```

## What It Does

Rounds each timestamp to the nearest boundary of the requested unit.

This is the midpoint-aware counterpart to [`floor`](floor.md) and
[`ceil`](ceil.md).

## Parameters

**unit**

The unit to round to, such as `"H"` for hours.

## Returns

`Timeline`
    A rounded timeline.

## Examples

Example 1. Round timestamps to the nearest hour.

```cpp
auto out = base.round("H");
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 09:00:00, 2024-11-01 10:00:00, 2025-01-01 00:00:00, 2025-01-01 00:00:00 ]
```

## Notes

- Use `round()` when neither always-down nor always-up behavior is right for
  the task.

## See Also

- [`Timeline::floor`](floor.md)
- [`Timeline::ceil`](ceil.md)
- [`Timeline::normalize`](normalize.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
