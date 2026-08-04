# TimeIndex::union_

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::union_(const TimeIndex& other) const -> TimeIndex
TimeIndex::unionIndex(const TimeIndex& other) const -> TimeIndex
```

## What It Does

Returns the sorted union of both time axes.

Every timestamp present on either side appears once in the result.

## Parameters

**other**

The other time index.

## Returns

`TimeIndex`
    The union axis.

## Examples

Example 1. Build the complete exact clock represented by both axes.

```cpp
auto out = base.union_(right);
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[7], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:05:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00, 2024-11-01 01:05:00, 2024-11-01 01:20:00 ]
```

Example 2. Use `union_()` when the shared exact clock itself is the object you
need, not the indexer maps.

```cpp
auto shared_clock = base.union_(right);
std::cout << pluss::print_timeline(shared_clock) << "\n";
```

## Notes

- Matching timestamps appear once in the result.
- If you also need left/right positional maps into the shared clock, use
  [`align_union`](align_union.md).

## See Also

- [`TimeIndex::intersection`](intersection.md)
- [`TimeIndex::difference`](difference.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="intersection.html">
    <span class="label">Next</span>
    <span class="title">intersection</span>
  </a>
</div>
