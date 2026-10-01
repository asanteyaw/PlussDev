# TimelineIndexAccessor::round

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
TimelineIndexAccessor TimelineIndexAccessor::round(std::string_view unit) const
```

## What It Does

Rounds each timestamp to the nearest requested unit.

This is the symmetric rounding form. Use it when neither earlier nor later
bucket ownership is inherently privileged.

## Parameters

**unit**

The target rounding unit.

## Returns

`TimelineIndexAccessor`
    The rounded axis.

## Examples

Example 1. Snap each timestamp to the nearest hour boundary.

```cpp
auto out = base.round("H");
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00 ]
```

Example 2. Choose `round()` when the closest boundary is what matters.

```cpp
auto nearest_hours = base.round("H");
std::cout << pluss::timeindex::print(nearest_hours) << "\n";
```

## See Also

- [`TimelineIndexAccessor::floor`](floor.md)
- [`TimelineIndexAccessor::ceil`](ceil.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_sum.html">
    <span class="label">Previous</span>
    <span class="title">rolling_sum</span>
  </a>
  <a class="next-link" href="searchsorted.html">
    <span class="label">Next</span>
    <span class="title">searchsorted</span>
  </a>
</div>
