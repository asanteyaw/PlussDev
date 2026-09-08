# Timeline::searchsorted_right

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::searchsorted_right(const Timeline& values) const
```

## Find The End Of Each Insertion Range

`searchsorted_right(values)` returns the latest position where each query could
be inserted into a sorted 1-D receiver without disturbing ascending order. An
equal query is placed after every existing match.

A query beyond the valid range returns the beginning of the trailing `NaT`
group. Query `NaT` itself returns the full receiver length, after all existing
absent entries. The Int64 result has exactly the query's shape.

## Example 1: Observe Right-Edge Tie Handling

```cpp
const auto sorted = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-06-01T00:00:00Z",
    "2024-06-01T00:00:00Z", "2025-01-01T00:00:00Z", "", "",
});
const auto queries = pluss::timeline::make({
    {"2023-12-01T00:00:00Z", "2024-06-01T00:00:00Z"},
    {"2024-08-01T00:00:00Z", ""},
});
std::cout << pluss::series::print(sorted.searchsorted_right(queries)) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0 3 ⎤
⎣ 3 6 ⎦
```

The equal June query returns `3`, after both matches. Query `NaT` returns `6`,
after the two existing absent entries.

## Example 2: Test Exact Membership

```cpp
const auto key = pluss::timeline::make("2024-07-01T00:00:00Z");
const auto left = sorted.searchsorted_left(key).item<std::int64_t>();
const auto right = sorted.searchsorted_right(key).item<std::int64_t>();
std::cout << std::boolalpha << "found=" << (right > left) << '\n';
```

### Output

```text
found=false
```

Both methods return the same insertion point for an absent key. For a duplicate
key, their difference is the exact match count.

## Choosing A Side

- Use `searchsorted_left()` to insert before equals or find the first match.
- Use `searchsorted_right()` to insert after equals or find one-past-last match.
- Use both to produce a half-open equal range and count duplicates.

## Preconditions And Failure Modes

- The receiver must be 1-D, ascending, and have any `NaT` entries at the end.
- The method does not validate or repair arbitrary unsorted input.
- A result can equal `numel()`: that is a legal insertion boundary, not a legal
  existing-element index.

## Summary

`searchsorted_right()` returns the boundary after equal values, including after
all existing `NaT` values, while preserving the query shape.

## See Also

- [`searchsorted_left`](searchsorted_left.md)
- [`sort`](sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="searchsorted_left.html"><span class="label">Previous</span><span class="title">searchsorted_left</span></a>
  <a class="next-link" href="fill_nat.html"><span class="label">Next</span><span class="title">fill_nat</span></a>
</div>
