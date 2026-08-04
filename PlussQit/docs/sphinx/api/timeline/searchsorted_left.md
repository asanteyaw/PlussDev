# Timeline::searchsorted_left

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline::searchsorted_left(const Timeline& values) -> Series
Timeline::searchsorted_right(const Timeline& values) -> Series
Timeline::to_epoch_nanos_flat() -> std::vector<std::int64_t>
```

This page covers the ordered insertion and raw tick-export family.

## What These Methods Do

`searchsorted_left(...)` and `searchsorted_right(...)` answer the question:
"if I inserted these query timestamps into this already-sorted timeline, at
what positions would they land?"

That makes them useful for:

- ordered lookup preparation
- bin-edge reasoning
- merge/as-of style temporal alignment
- converting exact-or-nearest timestamp reasoning into integer positions

`to_epoch_nanos_flat()` is the low-level export surface that exposes the raw
epoch-nanosecond representation as a flat integer vector.

## Parameters

**values**

The timestamps whose insertion positions you want to compute.

The shape does not need to match the source timeline length; the result length
matches the number of query timestamps you provide.

The behavioral toggle is the side rule:

- `searchsorted_left(...)`
  Insert before existing equal timestamps.
- `searchsorted_right(...)`
  Insert after existing equal timestamps.

That difference only matters when the query timestamp is already present.

## Returns

`Series`
    Integer insertion positions for the search methods.

`std::vector<std::int64_t>`
    Raw epoch-nanosecond ticks for `to_epoch_nanos_flat()`.

## Notes

- The search methods are meant for sorted 1D timelines.
- Left versus right matters exactly at equal-key boundaries.
- `to_epoch_nanos_flat()` is intentionally lower level: use it when you need to
  interoperate with external systems or numerical kernels that expect raw tick
  values instead of `Timeline`.

## Examples

Example 1. Compute left insertion positions for queries that land before,
inside, and after the sorted range.

```cpp
auto pos = sorted.searchsorted_left(queries);
std::cout << pluss::print_series(pos) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 2, 4]
```

Example 2. Switch to right insertion positions so equal timestamps fall after
existing matches.

```cpp
auto pos = sorted.searchsorted_right(queries);
std::cout << pluss::print_series(pos) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 4, 4]
```

Example 3. See the equal-key difference directly on timestamps that already
exist in the sorted timeline.

```cpp
auto left = sorted.searchsorted_left(pluss::timeline::make({
    "2024-11-01T09:00:00Z",
    "2024-11-01T11:00:00Z",
}));
std::cout << pluss::print_series(left) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 2]
```

Example 4. Use the right-side rule on the same exact matches.

```cpp
auto right = sorted.searchsorted_right(pluss::timeline::make({
    "2024-11-01T09:00:00Z",
    "2024-11-01T11:00:00Z",
}));
std::cout << pluss::print_series(right) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 4]
```

Example 5. Export the same timeline as raw epoch nanoseconds.

```cpp
auto ticks = base.to_epoch_nanos_flat();
std::cout << "[";
for (std::size_t i = 0; i < ticks.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << ticks[i];
}
std::cout << "]\n";
```

### Output

```text
[1730451600000000000, 1730456130000000000, 1735689599000000000, 1735689600000000000]
```

## See Also

- [`TimeIndex::searchsorted`](../timeindex/searchsorted.md)
- [`Timeline::argsort`](argsort.md)
- [`Timeline::min`](min.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_nanos.html">
    <span class="label">Previous</span>
    <span class="title">add_nanos</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
