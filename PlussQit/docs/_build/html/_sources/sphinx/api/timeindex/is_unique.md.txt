# TimeIndex::is_unique

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::is_unique() const -> bool
TimeIndex::has_duplicates() const -> bool
```

## What It Does

Reports whether the index has duplicate timestamps.

`has_duplicates()` is the inverse convenience surface.

This is the first question to ask when exact one-timestamp-per-case semantics
matter.

## Returns

`bool`
    `true` if the uniqueness predicate holds for the chosen method.

## Examples

Example 1. A standard ordered axis with no repeated timestamps.

```cpp
auto idx = pluss::make_timeindex({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});
std::cout << "idx.is_unique() = " << (idx.is_unique() ? "true" : "false") << "\n";
std::cout << "idx.has_duplicates() = "
          << (idx.has_duplicates() ? "true" : "false") << "\n";
```

### Output

```text
idx.is_unique() = true
idx.has_duplicates() = false
```

Example 2. Detect duplicates in an intentionally permissive unsorted index.

```cpp
auto dup = pluss::make_timeindex({
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:00:00Z",
}, false);
std::cout << "dup.is_unique() = " << (dup.is_unique() ? "true" : "false") << "\n";
std::cout << "dup.has_duplicates() = "
          << (dup.has_duplicates() ? "true" : "false") << "\n";
```

### Output

```text
dup.is_unique() = false
dup.has_duplicates() = true
```

## See Also

- [`TimeIndex::unique`](unique.md)
- [`TimeIndex::sort`](sort.md)

## Notes

- Uniqueness and monotonicity are different checks. An index can be monotonic
  and still contain duplicates.
- Duplicates are not inherently wrong, but they change how you should think
  about exact lookup and regrouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_monotonic_increasing.html">
    <span class="label">Previous</span>
    <span class="title">is_monotonic_increasing</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
