# TimeIndex::ceil

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::ceil(std::string_view unit) const -> TimeIndex
```

## What It Does

Rounds each timestamp up to the requested unit.

Unit-specific wrappers mirror the generic method:

- `ceil_ns()`, `ceil_us()`, `ceil_ms()`, `ceil_s()`
- `ceil_min()`, `ceil_h()`, `ceil_D()`, `ceil_W()`

Use this when later bucket ownership should prefer the next boundary rather than
the previous one.

## Parameters

**unit**

The target rounding unit.

## Returns

`TimeIndex`
    The rounded axis.

## Examples

Example 1. Snap every timestamp up to the containing or next hour boundary.

```cpp
auto out = base.ceil("H");
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 02:00:00 ]
```

Example 2. Prefer `ceil()` when later boundary ownership is the intended
semantic.

```cpp
auto next_boundaries = base.ceil("H");
```

## Notes

- `ceil()` always rounds upward in the chosen unit family.

## See Also

- [`TimeIndex::floor`](floor.md)
- [`TimeIndex::round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
