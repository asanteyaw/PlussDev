# Timeline::sort

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::sort() const
```

## Order Instants Chronologically

`sort()` returns a new one-dimensional Timeline in ascending temporal order.
It compares stored instants at nanosecond precision, preserves the receiver's
timezone, and leaves the receiver unchanged.

Sorting is stable. When two entries represent the same instant, their relative
input order is retained. All valid instants precede `NaT`; multiple absent
entries remain at the end in their original order. This makes the result a
valid receiver for the two `searchsorted` methods.

## Example 1: Sort Repeated And Absent Instants

```cpp
const auto input = pluss::timeline::make({
    "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
    "2025-03-01T00:00:00Z", "", "2024-06-01T12:00:00.123456789Z",
});

std::cout << pluss::timeline::print(input.sort()) << '\n';
```

### Output

```text
Timeline(shape=[6], tz=UTC)
[ 2024-01-01 00:00:00, 2024-06-01 12:00:00, 2025-03-01 00:00:00, 2025-03-01 00:00:00, NaT, NaT ]
```

The default printer shows whole seconds, but the June timestamp still carries
its `.123456789` fractional component internally. Ordering does not round it.

## Example 2: Confirm That Sorting Is Out Of Place

```cpp
const auto sorted = input.sort();
std::cout << pluss::timeline::print(input) << '\n';
std::cout << pluss::timeline::print(sorted) << '\n';
```

### Output

```text
Timeline(shape=[6], tz=UTC)
[ 2025-03-01 00:00:00, NaT, 2024-01-01 00:00:00, 2025-03-01 00:00:00, NaT, 2024-06-01 12:00:00 ]
Timeline(shape=[6], tz=UTC)
[ 2024-01-01 00:00:00, 2024-06-01 12:00:00, 2025-03-01 00:00:00, 2025-03-01 00:00:00, NaT, NaT ]
```

## Choosing Between Ordering Operations

- `sort()` returns all entries reordered, including duplicates and `NaT`.
- `argsort()` returns the positions that describe that reorder.
- `unique()` removes repeated values while preserving first-seen order; it does
  not sort.
- `min()` and `max()` reduce the entire Timeline to one scalar instant.

## Gotchas And Failure Modes

- Only a 1-D Timeline is accepted.
- There is no descending overload on this Timeline surface.
- The timezone affects rendering, not the chronological comparison.

## Summary

`sort()` is a stable, ascending, out-of-place temporal sort with `NaT` last and
timezone and nanosecond identity preserved.

## See Also

- [`argsort`](argsort.md)
- [`unique`](unique.md)
- [`searchsorted_left`](searchsorted_left.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html"><span class="label">Previous</span><span class="title">max</span></a>
  <a class="next-link" href="span.html"><span class="label">Next</span><span class="title">span</span></a>
</div>
