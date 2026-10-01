# TimelineIndexAccessor::take

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
TimelineIndexAccessor TimelineIndexAccessor::take(const pluss::Series& indices) const
```

## What It Does

Selects timestamps by integer position.

Use this when you already have an explicit position vector such as an `argsort`
result or a custom gather order.

`take()` is the general position-based gather surface.

## Parameters

**indices**

A 1D integer `Series` of positions to gather.

## Returns

`TimelineIndexAccessor`
    A re-ordered or subset index.

## Examples

```cpp
auto out = base.take(pluss::make_series(std::vector<std::int64_t>{4, 2, 0}));
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-11-01 01:20:00, 2024-11-01 00:40:00, 2024-11-01 00:00:00 ]
```

Example 2. Reuse an `argsort()` result to physically reorder the axis.

```cpp
auto order = dup.argsort();
auto sorted = dup.take(order);
std::cout << pluss::series::print(order) << "\n";
std::cout << pluss::timeindex::print(sorted) << "\n";
```

## Notes

- `take()` is explicit-position logic. It does not interpret the integers as
  timestamps or labels.

## See Also

- [`TimelineIndexAccessor::argsort`](argsort.md)
- [`TimelineIndexAccessor::slice_index`](slice_index.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
