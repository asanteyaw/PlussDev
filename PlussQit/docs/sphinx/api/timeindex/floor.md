# TimeIndex::floor

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::floor(std::string_view unit) const -> TimeIndex
```

## What It Does

Rounds each timestamp down to the requested unit.

Unit-specific wrappers also exist for discoverability:

- `floor_ns()`
- `floor_us()`
- `floor_ms()`
- `floor_s()`
- `floor_min()`
- `floor_h()`
- `floor_D()`
- `floor_W()`

Use this when every timestamp should snap to the start of its containing bucket.

## Parameters

**unit**

The target rounding unit such as `"H"`, `"min"`, `"D"`, or `"W"`.

The case count stays the same. Only the timestamp values change.

## Returns

`TimeIndex`
    The rounded axis.

## Examples

Example 1. Snap every timestamp down to the containing hour boundary.

```cpp
auto out = base.floor("H");
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00 ]
```

Example 2. Prefer `floor()` when earlier bucket ownership is the intended
semantic.

```cpp
auto bucket_starts = base.floor("H");
```

## Notes

- `floor()` always rounds downward in the chosen unit family.

## See Also

- [`TimeIndex::ceil`](ceil.md)
- [`TimeIndex::round`](round.md)
- [`TimeIndex::normalize`](normalize.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="difference.html">
    <span class="label">Previous</span>
    <span class="title">difference</span>
  </a>
  <a class="next-link" href="ceil.html">
    <span class="label">Next</span>
    <span class="title">ceil</span>
  </a>
</div>
