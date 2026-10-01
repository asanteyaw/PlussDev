# TimelineIndexAccessor::is_monotonic_increasing

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
bool TimelineIndexAccessor::is_monotonic_increasing() const
```

## What It Does

Reports whether every timestamp is greater than or equal to the timestamp
before it. Repeated instants are permitted; monotonicity concerns direction,
not uniqueness.

This matters because time-aware lookup methods such as `searchsorted`, `asof`,
and most resampling logic expect an ordered axis.

If the axis is not monotonic in the direction you think it is, downstream time
semantics become harder to reason about immediately.

## Returns

A native C++ `bool`. Empty and single-element indices return `true` because no
adjacent pair violates increasing order.

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
```

### Output

```text
idx.is_monotonic_increasing() = true
```

Example 2. A descending axis fails the increasing check when construction-time
order enforcement is intentionally disabled.

```cpp
auto descending = pluss::timeindex::make({
    "2024-11-01T01:20:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:00:00Z",
}, false);
std::cout << "descending.is_monotonic_increasing() = "
          << (descending.is_monotonic_increasing() ? "true" : "false") << "\n";
```

### Output

```text
descending.is_monotonic_increasing() = false
```

## Notes

- Most production `TimelineIndexAccessor` usage should stay monotonic increasing.
- Use [`is_monotonic_decreasing()`](is_monotonic_decreasing.md) when a
  newest-first axis is intentional.
- A non-monotonic index is not just a cosmetic issue. It changes the meaning of
  ordered lookup and can invalidate assumptions in time-aware code.

## See Also

- [`TimelineIndexAccessor::searchsorted`](searchsorted.md)
- [`TimelineIndexAccessor::asof`](asof.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_monotonic_decreasing.html">
    <span class="label">Previous</span>
    <span class="title">is_monotonic_decreasing</span>
  </a>
  <a class="next-link" href="is_unique.html">
    <span class="label">Next</span>
    <span class="title">is_unique</span>
  </a>
</div>
