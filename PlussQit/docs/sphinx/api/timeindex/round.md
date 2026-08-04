# TimeIndex::round

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::round(std::string_view unit) const -> TimeIndex
```

## What It Does

Rounds each timestamp to the nearest requested unit.

Wrapper families mirror the generic method:

- `round_ns()`, `round_us()`, `round_ms()`, `round_s()`
- `round_min()`, `round_h()`, `round_D()`, `round_W()`

This is the symmetric rounding form. Use it when neither earlier nor later
bucket ownership is inherently privileged.

## Parameters

**unit**

The target rounding unit.

## Returns

`TimeIndex`
    The rounded axis.

## Examples

Example 1. Snap each timestamp to the nearest hour boundary.

```cpp
auto out = base.round("H");
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00, 2024-11-01 01:00:00 ]
```

Example 2. Choose `round()` when the closest boundary is what matters.

```cpp
auto nearest_hours = base.round("H");
std::cout << pluss::print_timeline(nearest_hours) << "\n";
```

## See Also

- [`TimeIndex::floor`](floor.md)
- [`TimeIndex::ceil`](ceil.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
