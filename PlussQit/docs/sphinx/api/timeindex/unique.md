# TimeIndex::unique

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::unique() const -> TimeIndex
TimeIndex::drop_duplicates() const -> TimeIndex
```

## What It Does

Returns the distinct timestamps in ascending order.

`drop_duplicates()` is the same operation under a more data-cleaning-oriented
name.

This gives you the canonical exact-time set represented by the axis.

## Returns

`TimeIndex`
    A deduplicated index.

## Examples

```cpp
auto dup = pluss::make_timeindex({
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:00:00Z",
}, false);

auto out = dup.unique();
std::cout << pluss::print_timeline(out) << "\n";
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
std::cout << pluss::print_timeline(vocab) << "\n";
```

## Notes

- `unique()` removes repetition and returns a sorted canonical axis.
- If you only want to test for duplicates without materializing a new axis,
  use `is_unique()` or `has_duplicates()`.

## See Also

- [`TimeIndex::is_unique`](is_unique.md)
- [`TimeIndex::sort`](sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="argsort.html">
    <span class="label">Next</span>
    <span class="title">argsort</span>
  </a>
</div>
