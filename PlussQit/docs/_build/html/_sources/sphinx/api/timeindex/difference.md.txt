# TimeIndex::difference

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::difference(const TimeIndex& other) const -> TimeIndex
TimeIndex::diffIndex(const TimeIndex& other) const -> TimeIndex
```

## What It Does

Returns the timestamps present in `self` but not in `other`.

This is the left-only exact subtraction on the axis.

## Parameters

**other**

The other time index.

## Returns

`TimeIndex`
    The left-only timestamps.

## Examples

Example 1. Remove every timestamp that also appears in the other axis.

```cpp
auto out = base.difference(right);
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00 ]
```

Example 2. Use `difference()` to inspect the timestamps that remain unmatched
after exact overlap has been removed.

```cpp
auto left_only = base.difference(right);
```

## Notes

- The operation is directional: `a.difference(b)` is not the same thing as
  `b.difference(a)`.

## See Also

- [`TimeIndex::union_`](union_.md)
- [`TimeIndex::intersection`](intersection.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersection.html">
    <span class="label">Previous</span>
    <span class="title">intersection</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
