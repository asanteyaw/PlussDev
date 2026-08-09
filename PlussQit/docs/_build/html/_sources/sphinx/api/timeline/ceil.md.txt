# Timeline::ceil

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::ceil(std::string_view unit) -> Timeline
```

## What It Does

Rounds each timestamp up to the next boundary of the requested unit.

## Parameters

**unit**

The target unit boundary, such as `"H"` for hourly rounding.

## Returns

`Timeline`
    A timeline of ceiled timestamps.

## Examples

Example 1. Ceiling to the hour.

```cpp
auto out = base.ceil("H");
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 09:00:00, 2024-11-01 11:00:00, 2025-01-01 00:00:00, 2025-01-01 00:00:00 ]
```

## Notes

- Ceil may leave already-aligned timestamps unchanged.
- It is useful for upper bucket boundaries and forward calendar binning.

## See Also

- [`Timeline::floor`](floor.md)
- [`Timeline::round`](round.md)

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
