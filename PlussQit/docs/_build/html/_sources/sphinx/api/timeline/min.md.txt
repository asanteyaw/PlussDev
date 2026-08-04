# Timeline::min

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline::min() -> Timeline
Timeline::max() -> Timeline
Timeline::sort() -> Timeline
Timeline::unique() -> Timeline
Timeline::drop_duplicates() -> Timeline
```

## What It Does

This page covers the ordering and deduplication family:

- `min()`
  Return the earliest defined timestamp as a scalar timeline.
- `max()`
  Return the latest defined timestamp as a scalar timeline.
- `sort()`
  Return the sorted 1D timeline.
- `unique()`
  Return distinct timestamps.
- `drop_duplicates()`
  The same deduplication result as `unique()` in the current implementation.

## Examples

Example 1. Read the earliest and latest defined timestamps.

```cpp
auto lo = duplicated.min();
auto hi = duplicated.max();
std::cout << pluss::print_timeline(lo) << "\n";
std::cout << pluss::print_timeline(hi) << "\n";
```

### Output

`duplicated.min()`:

```text
Timeline(shape=[], tz=UTC)
2024-11-01 09:00:00
```

`duplicated.max()`:

```text
Timeline(shape=[], tz=UTC)
2025-01-01 00:00:00
```

Example 2. Sort the timeline and then remove duplicates.

```cpp
auto ordered = duplicated.sort();
auto distinct = duplicated.unique();
auto deduped = duplicated.drop_duplicates();
std::cout << pluss::print_timeline(ordered) << "\n";
std::cout << pluss::print_timeline(distinct) << "\n";
std::cout << pluss::print_timeline(deduped) << "\n";
```

### Output

`duplicated.sort()`:

```text
Timeline(shape=[6], tz=UTC)
[ 2024-11-01 09:00:00, 2024-11-01 10:15:30, 2024-11-01 10:15:30, 2025-01-01 00:00:00, NaT, NaT ]
```

`duplicated.unique()`:

```text
Timeline(shape=[4], tz=UTC)
[ NaT, 2024-11-01 09:00:00, 2024-11-01 10:15:30, 2025-01-01 00:00:00 ]
```

`duplicated.drop_duplicates()`:

```text
Timeline(shape=[4], tz=UTC)
[ NaT, 2024-11-01 09:00:00, 2024-11-01 10:15:30, 2025-01-01 00:00:00 ]
```

## Notes

- Missing timestamps are ignored by `min()` and `max()` when searching for
  extrema.
- `sort()`, `unique()`, and `drop_duplicates()` are currently 1D-oriented
  operations.
- `unique()` currently returns sorted distinct values rather than preserving
  first-occurrence order.

## See Also

- [`Timeline::argsort`](argsort.md)
- [`Timeline::searchsorted_left`](searchsorted_left.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_utc.html">
    <span class="label">Previous</span>
    <span class="title">to_utc</span>
  </a>
  <a class="next-link" href="fill_nat.html">
    <span class="label">Next</span>
    <span class="title">fill_nat</span>
  </a>
</div>
