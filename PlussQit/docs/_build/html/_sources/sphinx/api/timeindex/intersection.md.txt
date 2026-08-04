# TimeIndex::intersection

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::intersection(const TimeIndex& other) const -> TimeIndex
TimeIndex::intersect(const TimeIndex& other) const -> TimeIndex
```

## What It Does

Returns the timestamps present in both axes.

This is the exact overlap axis. Near misses do not survive.

## Parameters

**other**

The other time index.

## Returns

`TimeIndex`
    The shared timestamps.

## Examples

Example 1. Keep only timestamps that appear on both sides exactly.

```cpp
auto out = base.intersection(right);
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 00:20:00, 2024-11-01 01:20:00 ]
```

Example 2. Use `intersection()` when only shared exact timestamps should remain
for the next step.

```cpp
auto overlap = base.intersection(right);
```

## Notes

- `intersection()` is strict exact-time set logic, not nearest-time logic.

## See Also

- [`TimeIndex::union_`](union_.md)
- [`TimeIndex::difference`](difference.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="union_.html">
    <span class="label">Previous</span>
    <span class="title">union_</span>
  </a>
  <a class="next-link" href="difference.html">
    <span class="label">Next</span>
    <span class="title">difference</span>
  </a>
</div>
