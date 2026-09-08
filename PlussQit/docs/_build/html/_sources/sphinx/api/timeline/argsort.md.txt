# Timeline::argsort

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::argsort() const
```

## Compute An Ordering Without Moving The Timestamps

`argsort()` returns the zero-based positions that would order a one-dimensional
Timeline in ascending chronological order. The timestamps themselves are not
returned and the receiver is not changed.

This indirection is useful whenever timestamps are the ordering key for another
collection. Compute the permutation once, then apply the same positions to each
aligned payload so their row relationships survive the reorder.

The result always has integer dtype and the same number of elements as the
receiver. Equal timestamps use a stable order: an earlier occurrence in the
input remains earlier in the permutation. Valid timestamps come first and all
`NaT` positions come last, also in their original relative order.

## Example 1: Read The Permutation

```cpp
const auto input = pluss::timeline::make({
    "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
    "2025-03-01T00:00:00Z", "", "2024-06-01T12:00:00.123456789Z",
});

std::cout << pluss::series::print(input.argsort()) << '\n';
```

### Output

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 5, 0, 3, 1, 4]
```

Position `2` comes first because it contains the earliest timestamp. Positions
`0` and `3` hold equal timestamps and remain in input order. Absent positions
`1` and `4` form the final stable group.

## Example 2: Contrast Positions With Values

```cpp
std::cout << pluss::series::print(input.argsort()) << '\n';
std::cout << pluss::timeline::print(input.sort()) << '\n';
```

### Output

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 5, 0, 3, 1, 4]
Timeline(shape=[6], tz=UTC)
[ 2024-01-01 00:00:00, 2024-06-01 12:00:00, 2025-03-01 00:00:00, 2025-03-01 00:00:00, NaT, NaT ]
```

Choose `argsort()` when positions are the product you need. Choose `sort()`
when the reordered Timeline itself is the product.

## Gotchas And Failure Modes

- The receiver must be 1-D. A scalar, matrix, or higher-rank Timeline throws.
- The order is chronological by stored instant, not lexicographic by rendered text.
- `NaT` is placed last; it is not treated as the earliest possible integer tick.
- The returned positions refer to the original receiver and are invalid for an
  unrelated or differently sized collection.

## Summary

`argsort()` produces a stable ascending permutation, preserves tie order, puts
absent instants last, and leaves the receiver untouched.

## See Also

- [`sort`](sort.md): return the reordered timestamps.
- [`searchsorted_left`](searchsorted_left.md): locate insertion boundaries.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat_scalar.html"><span class="label">Previous</span><span class="title">isnat_scalar</span></a>
  <a class="next-link" href="isoformat.html"><span class="label">Next</span><span class="title">isoformat</span></a>
</div>
