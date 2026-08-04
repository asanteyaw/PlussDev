# TimeIndex::max

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::max() const -> Timeline
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
std::cout << pluss::print_timeline(out) << "\n";
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

- [`TimeIndex::min`](min.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
