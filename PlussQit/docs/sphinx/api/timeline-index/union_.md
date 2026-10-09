# Timeline::TimelineIndexAccessor::union_

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::union_(const Timeline::TimelineIndexAccessor& other) const
```

## What It Does

Returns the sorted union of both time axes.

Every timestamp present on either side appears once in the result.

## Parameters

**other**

The other time index.

## Returns

`Timeline::TimelineIndexAccessor`
    The union axis.

## Examples

Example 1. Build the complete exact clock represented by both axes.

```cpp
auto out = base.union_(right);
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[7], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:05:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00, 2024-11-01 01:05:00, 2024-11-01 01:20:00 ]
```

Example 2. Use `union_()` when the shared exact clock itself is the object you
need, not the indexer maps.

```cpp
auto shared_clock = base.union_(right);
std::cout << pluss::timeline::print(shared_clock.timeline()) << "\n";
```

## Notes

- Matching timestamps appear once in the result.
- If you also need left/right positional maps into the shared clock, use
  [`align_union`](align_union.md).

## See Also

- [`Timeline::TimelineIndexAccessor::intersection`](intersection.md)
- [`Timeline::TimelineIndexAccessor::difference`](difference.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz_localize.html">
    <span class="label">Previous</span>
    <span class="title">tz_localize</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
