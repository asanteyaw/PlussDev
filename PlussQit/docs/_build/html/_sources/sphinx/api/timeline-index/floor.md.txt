# Timeline::TimelineIndexAccessor::floor

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds each timestamp down to the requested unit.

Use this when every timestamp should snap to the start of its containing bucket.

## Parameters

**unit**

The target rounding unit such as `"H"`, `"min"`, `"D"`, or `"W"`.

The case count stays the same. Only the timestamp values change.

## Returns

`Timeline::TimelineIndexAccessor`
    The rounded axis.

## Examples

Example 1. Snap every timestamp down to the containing hour boundary.

```cpp
auto out = base.floor("H");
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00 ]
```

Example 2. Prefer `floor()` when earlier bucket ownership is the intended
semantic.

```cpp
auto bucket_starts = base.floor("H");
```

## Notes

- `floor()` always rounds downward in the chosen unit family.

## See Also

- [`Timeline::TimelineIndexAccessor::ceil`](ceil.md)
- [`Timeline::TimelineIndexAccessor::round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="difference.html">
    <span class="label">Previous</span>
    <span class="title">difference</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
