# Timeline::searchsorted_left

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::searchsorted_left(const Timeline& values) const
```

## Find The Start Of Each Insertion Range

`searchsorted_left(values)` performs binary searches in a sorted 1-D receiver.
For every query timestamp, it returns the earliest position where that value
could be inserted without disturbing ascending order.

If a query equals existing timestamps, the answer points before the first
equal entry. If it falls between timestamps, the answer points at the later
neighbor. Query `NaT` points to the beginning of the trailing `NaT` group.

The receiver and query play different shape roles: the receiver must be 1-D,
but `values` may have any Timeline shape. The returned Int64 `Series` preserves
the query shape.

## Example 1: Search A Matrix Of Queries

```cpp
const auto sorted = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-06-01T00:00:00Z",
    "2024-06-01T00:00:00Z", "2025-01-01T00:00:00Z", "", "",
});
const auto queries = pluss::timeline::make({
    {"2023-12-01T00:00:00Z", "2024-06-01T00:00:00Z"},
    {"2024-08-01T00:00:00Z", ""},
});
std::cout << pluss::series::print(sorted.searchsorted_left(queries)) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0 1 ⎤
⎣ 3 4 ⎦
```

The equal June query returns `1`, before both matches. The absent query returns
`4`, before both trailing `NaT` entries.

## Example 2: Use Both Sides As A Match Range

```cpp
const auto key = pluss::timeline::make("2024-06-01T00:00:00Z");
const auto begin = sorted.searchsorted_left(key).item<std::int64_t>();
const auto end = sorted.searchsorted_right(key).item<std::int64_t>();
std::cout << "begin=" << begin << ", end=" << end
          << ", count=" << (end - begin) << '\n';
```

### Output

```text
begin=1, end=3, count=2
```

The half-open interval `[begin, end)` identifies every exact match without a
linear scan.

## Preconditions And Failure Modes

- The receiver must be 1-D and already ascending with `NaT` last. Call `sort()`
  first when necessary.
- The method does not sort or validate arbitrary receiver order.
- The query may be scalar or N-D; only the receiver has the 1-D restriction.
- Results are insertion positions, not proof that an exact match exists.

## Summary

`searchsorted_left()` returns the first insertion boundary for each query,
preserves query shape, and resolves ties and `NaT` to the left edge.

## See Also

- [`searchsorted_right`](searchsorted_right.md)
- [`sort`](sort.md)
- [`argsort`](argsort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html"><span class="label">Previous</span><span class="title">unique</span></a>
  <a class="next-link" href="searchsorted_right.html"><span class="label">Next</span><span class="title">searchsorted_right</span></a>
</div>
