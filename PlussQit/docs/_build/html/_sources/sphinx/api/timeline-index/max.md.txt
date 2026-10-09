# Timeline::TimelineIndexAccessor::max

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
Timeline Timeline::TimelineIndexAccessor::max() const
```

## What It Does

Returns the latest timestamp in the index as a scalar `Timeline`.

This is the right edge of the observed axis.

## Returns

`Timeline`
    A scalar temporal value containing the maximum timestamp.

## Examples

Example 1. Read the last instant represented anywhere on the axis.

```cpp
auto out = idx.max();
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-11-01 01:20:00
```

Example 2. Use `max()` when you need to bound later operations against the
visible end of the axis.

```cpp
auto hi = idx.max();
```

## See Also

- [`Timeline::TimelineIndexAccessor::min`](min.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join_asof.html">
    <span class="label">Previous</span>
    <span class="title">join_asof</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
