<!-- pluss-hand-authored -->

# Duration::total().minutes()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::minutes() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
minutes — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`60 * 1e9`. See [`total().nanoseconds()`](total_nanoseconds.md) for the full
explanation of the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in minutes.

## Examples

Example 1. A `90`-minute `Duration` read back as minutes.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::print_series(ninety_min.total().minutes());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=90.0000
```

Round-trips back to `90`, confirming the construction and conversion agree.

## Notes

- `.total().minutes()` never rounds or truncates — the result is always the
  exact (floating-point) division.

## See Also

- [`total().seconds()`](total_seconds.md): the finer unit this divides.
- [`total().hours()`](total_hours.md): the next unit up.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_seconds.html">
    <span class="label">Previous</span>
    <span class="title">total_seconds</span>
  </a>
  <a class="next-link" href="total_hours.html">
    <span class="label">Next</span>
    <span class="title">total_hours</span>
  </a>
</div>
