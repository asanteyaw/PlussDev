# Timeline::argsort

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::argsort() -> Series
```

Returns the positions that would sort a 1D timeline into ascending temporal
order.

Use `argsort()` when you want the ordering information itself rather than the
sorted timestamps. That distinction matters when the timeline is acting as the
ordering key for some other payload that you plan to rearrange in the same way.

## Parameters

This method takes no parameters.

The practical choice is between two related operations:

- use `argsort()` when you need positions
- use [`sort()`](min.md) when you need the sorted `Timeline` itself

## Returns

`Series`
    An integer position vector describing the ascending sort order.

## Notes

- This method is meaningful for ordered 1D temporal vectors.
- In the current implementation, valid timestamps sort before `NaT`.
- Equal timestamps remain tied as equal keys; the returned positions tell you
  where those equal entries appear in ascending order, but not a semantic
  grouping beyond that.

## Examples

Example 1. Compute the sort order for an unsorted timeline that contains
duplicates and `NaT`.

```cpp
auto order = duplicated.argsort();
std::cout << pluss::print_series(order) << "\n";
```

### Output

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[1, 0, 2, 4, 3, 5]
```

Example 2. Call `argsort()` on a timeline that is already increasing.

```cpp
auto order = base.argsort();
std::cout << pluss::print_series(order) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3]
```

Example 3. Compare `argsort()` with sorting the timeline values themselves.

```cpp
auto order = duplicated.argsort();
auto sorted = duplicated.sort();
std::cout << pluss::print_series(order) << "\n";
std::cout << pluss::print_timeline(sorted) << "\n";
```

### Output

`argsort()` gives positions:

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[1, 0, 2, 4, 3, 5]
```

`sort()` gives the reordered timestamps:

```text
Timeline(shape=[6], tz=UTC)
[ 2024-11-01 09:00:00, 2024-11-01 10:15:30, 2024-11-01 10:15:30, 2025-01-01 00:00:00, NaT, NaT ]
```

## See Also

- [`Timeline::min`](min.md)
- [`Timeline::searchsorted_left`](searchsorted_left.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat_scalar.html">
    <span class="label">Previous</span>
    <span class="title">isnat_scalar</span>
  </a>
  <a class="next-link" href="isoformat.html">
    <span class="label">Next</span>
    <span class="title">isoformat</span>
  </a>
</div>
