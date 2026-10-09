# Timeline::TimelineIndexAccessor::unique

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
Timeline::TimelineIndexAccessor Timeline::TimelineIndexAccessor::unique() const
```

## What It Does

Returns the distinct timestamps in ascending order.


This gives you the canonical exact-time set represented by the axis.

## Returns

`Timeline::TimelineIndexAccessor`
    A deduplicated index.

## Examples

```cpp
auto dup = pluss::timeline::make({
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:00:00Z",
}).index(false);

auto out = dup.unique();
std::cout << pluss::timeline::print(out.timeline()) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00 ]
```

Example 2. Use `unique()` when you want a clean axis vocabulary before set
operations or exact alignment.

```cpp
auto vocab = dup.unique();
std::cout << pluss::timeline::print(vocab.timeline()) << "\n";
```

## Notes

- `unique()` removes repetition while retaining the first occurrence order.
  Call `sort()` afterward when a canonical increasing axis is required.
- If you only want to test for duplicates without materializing a new axis,
  negate `is_unique()`.

## See Also

- [`Timeline::TimelineIndexAccessor::is_unique`](is_unique.md)
- [`Timeline::TimelineIndexAccessor::sort`](sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="union_.html">
    <span class="label">Previous</span>
    <span class="title">union_</span>
  </a>
  <a class="next-link" href="../../timeline_index_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Timeline Index Accessor API Reference</span>
  </a>
</div>
