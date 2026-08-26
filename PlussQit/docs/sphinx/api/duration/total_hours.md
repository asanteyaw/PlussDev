<!-- pluss-hand-authored -->

# Duration::total().hours()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::hours() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
hours — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`3'600 * 1e9`. See [`total().nanoseconds()`](total_nanoseconds.md) for the
full explanation of the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in hours.

## Examples

Example 1. A `90`-minute `Duration` read as hours.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::series::print(ninety_min.total().hours());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.5000
```

`.total().hours()` gives the non-integer `1.5`, confirming this accessor
never rounds — `90` minutes is exactly `1.5` hours, not `1` or `2`.

## Notes

- `.total().hours()` never rounds or truncates — the result is always the
  exact (floating-point) division, which is why a `90`-minute `Duration`
  correctly reads `1.5`, not `1` or `2`.

## See Also

- [`total().minutes()`](total_minutes.md): the finer unit this divides.
- [`total().days()`](total_days.md): the next unit up.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_minutes.html">
    <span class="label">Previous</span>
    <span class="title">total_minutes</span>
  </a>
  <a class="next-link" href="total_days.html">
    <span class="label">Next</span>
    <span class="title">total_days</span>
  </a>
</div>
