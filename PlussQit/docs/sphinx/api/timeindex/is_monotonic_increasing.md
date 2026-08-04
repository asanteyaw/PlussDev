# TimeIndex::is_monotonic_increasing

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::is_monotonic_increasing() const -> bool
TimeIndex::is_monotonic_decreasing() const -> bool
```

## What It Does

Reports whether the index is monotonic in ascending or descending order.

This matters because time-aware lookup methods such as `searchsorted`, `asof`,
and most resampling logic expect an ordered axis.

If the axis is not monotonic in the direction you think it is, downstream time
semantics become harder to reason about immediately.

## Returns

`bool`
    `true` when the index follows the requested monotonic direction.

## Examples

Example 1. A standard ascending axis is monotonic increasing.

```cpp
auto idx = pluss::timeindex::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});
std::cout << "idx.is_monotonic_increasing() = "
          << (idx.is_monotonic_increasing() ? "true" : "false") << "\n";
std::cout << "idx.is_monotonic_decreasing() = "
          << (idx.is_monotonic_decreasing() ? "true" : "false") << "\n";
```

### Output

```text
idx.is_monotonic_increasing() = true
idx.is_monotonic_decreasing() = false
```

Example 2. A descending axis can still be checked when you intentionally build
it without sorted enforcement.

```cpp
auto descending = pluss::timeindex::make({
    "2024-11-01T01:20:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:00:00Z",
}, false);
std::cout << "descending.is_monotonic_decreasing() = "
          << (descending.is_monotonic_decreasing() ? "true" : "false") << "\n";
```

### Output

```text
descending.is_monotonic_decreasing() = true
```

## Notes

- Most production `TimeIndex` usage should stay monotonic increasing.
- Descending checks are still useful when validating user-provided or external
  axes.
- A non-monotonic index is not just a cosmetic issue. It changes the meaning of
  ordered lookup and can invalidate assumptions in time-aware code.

## See Also

- [`TimeIndex::searchsorted`](searchsorted.md)
- [`TimeIndex::asof`](asof.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html">
    <span class="label">Previous</span>
    <span class="title">host</span>
  </a>
  <a class="next-link" href="is_unique.html">
    <span class="label">Next</span>
    <span class="title">is_unique</span>
  </a>
</div>
