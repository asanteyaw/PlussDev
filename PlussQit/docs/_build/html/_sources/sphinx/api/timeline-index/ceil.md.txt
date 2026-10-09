# Timeline::TimelineIndexAccessor::ceil

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds each timestamp up to the requested unit.

Use this when later bucket ownership should prefer the next boundary rather than
the previous one.

## Parameters

**unit**

The target rounding unit.

## Returns

`Timeline::TimelineIndexAccessor`
    The rounded axis.

## Examples

Example 1. Snap every timestamp up to the containing or next hour boundary.

```cpp
auto out = base.ceil("H");
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 02:00:00 ]
```

Example 2. Prefer `ceil()` when later boundary ownership is the intended
semantic.

```cpp
auto next_boundaries = base.ceil("H");
```

## Notes

- `ceil()` always rounds upward in the chosen unit family.

## See Also

- [`Timeline::TimelineIndexAccessor::floor`](floor.md)
- [`Timeline::TimelineIndexAccessor::round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asof.html">
    <span class="label">Previous</span>
    <span class="title">asof</span>
  </a>
  <a class="next-link" href="difference.html">
    <span class="label">Next</span>
    <span class="title">difference</span>
  </a>
</div>
