# Timeline::TimelineIndexAccessor::min

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
Timeline Timeline::TimelineIndexAccessor::min() const
```

## What It Does

Returns the earliest timestamp in the index as a scalar `Timeline`.

This is the left edge of the observed axis.

## Returns

`Timeline`
    A scalar temporal value containing the minimum timestamp.

## Examples

Example 1. Read the first instant represented anywhere on the axis.

```cpp
auto out = idx.min();
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-11-01 00:00:00
```

Example 2. Use `min()` and `max()` together to summarize the temporal span
before resampling or joining.

```cpp
auto lo = idx.min();
auto hi = idx.max();
```

## See Also

- [`Timeline::TimelineIndexAccessor::max`](max.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="operators.html">
    <span class="label">Next</span>
    <span class="title">operators</span>
  </a>
</div>
