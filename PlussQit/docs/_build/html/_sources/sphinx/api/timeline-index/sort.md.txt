# Timeline::TimelineIndexAccessor::sort

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::sort() const
```

## What It Does

Returns the timestamps in ascending order.

Use this to canonicalize an axis before you rely on ordered semantics such as
`searchsorted`, `asof`, or downstream alignment.

## Returns

`Timeline::TimelineIndexAccessor`
    A sorted time index.

## Examples

Example 1. Sort an unsorted axis that also contains duplicates.

```cpp
auto dup = pluss::timeline::make({
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:00:00Z",
}).index(false);

auto out = dup.sort();
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:20:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00 ]
```

Example 2. Pair `argsort()` with `take()` when you need the same reordering
plan applied to other aligned payloads too.

```cpp
auto order = dup.argsort();
auto sorted_idx = dup.take(order);
std::cout << pluss::series::print(order) << "\n";
std::cout << pluss::timeline::print(sorted_idx.timeline()) << "\n";
```

## Notes

- Sorting preserves duplicates; it only changes order.
- If you want both ordering and deduplication, call `sort()` and `unique()`,
  or just `unique()` if the distinct sorted axis is the final goal.

## See Also

- [`Timeline::TimelineIndexAccessor::argsort`](argsort.md)
- [`Timeline::TimelineIndexAccessor::unique`](unique.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice_time.html">
    <span class="label">Previous</span>
    <span class="title">slice_time</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
