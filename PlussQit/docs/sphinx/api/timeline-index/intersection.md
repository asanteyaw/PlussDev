# Timeline::TimelineIndexAccessor::intersection

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::intersection(const Timeline::TimelineIndexAccessor& other) const
```

## What It Does

Returns the timestamps present in both axes.

This is the exact overlap axis. Near misses do not survive.

## Parameters

**other**

The other time index.

## Returns

`Timeline::TimelineIndexAccessor`
    The shared timestamps.

## Examples

Example 1. Keep only timestamps that appear on both sides exactly.

```cpp
auto out = base.intersection(right);
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 00:20:00, 2024-11-01 01:20:00 ]
```

Example 2. Use `intersection()` when only shared exact timestamps should remain
for the next step.

```cpp
auto overlap = base.intersection(right);
```

## Notes

- `intersection()` is strict exact-time set logic, not nearest-time logic.

## See Also

- [`Timeline::TimelineIndexAccessor::union_`](union_.md)
- [`Timeline::TimelineIndexAccessor::difference`](difference.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html">
    <span class="label">Previous</span>
    <span class="title">host</span>
  </a>
  <a class="next-link" href="is_monotonic_decreasing.html">
    <span class="label">Next</span>
    <span class="title">is_monotonic_decreasing</span>
  </a>
</div>
