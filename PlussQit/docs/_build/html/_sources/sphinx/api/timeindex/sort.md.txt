# TimeIndex::sort

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::sort() const -> TimeIndex
TimeIndex::sort_values() const -> TimeIndex
TimeIndex::sort_index() const -> TimeIndex
```

## What It Does

Returns the timestamps in ascending order.

`sort_values()` and `sort_index()` are naming aliases for the same operation.

Use this to canonicalize an axis before you rely on ordered semantics such as
`searchsorted`, `asof`, or downstream alignment.

## Returns

`TimeIndex`
    A sorted time index.

## Examples

Example 1. Sort an unsorted axis that also contains duplicates.

```cpp
auto dup = pluss::timeindex::make({
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:00:00Z",
}, false);

auto out = dup.sort();
std::cout << pluss::timeindex::print(out) << "\n";
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
std::cout << pluss::print_series(order) << "\n";
std::cout << pluss::timeindex::print(sorted_idx) << "\n";
```

## Notes

- Sorting preserves duplicates; it only changes order.
- If you want both ordering and deduplication, call `sort()` and `unique()`,
  or just `unique()` if the distinct sorted axis is the final goal.

## See Also

- [`TimeIndex::argsort`](argsort.md)
- [`TimeIndex::unique`](unique.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
