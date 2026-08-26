<!-- pluss-hand-authored -->

# Duration::total().microseconds()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::microseconds() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
microseconds — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`1'000`. See [`total().nanoseconds()`](total_nanoseconds.md) for the full
explanation of the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in microseconds.

## Examples

Example 1. A `90`-minute `Duration` read as microseconds.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::series::print(ninety_min.total().microseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.4000e+09
```

`90` minutes is `5.4 * 10^9` microseconds.

## Notes

- `.total().microseconds()` never rounds or truncates — the result is always
  the exact (floating-point) division.

## See Also

- [`total().nanoseconds()`](total_nanoseconds.md): the finer unit this divides.
- [`total().milliseconds()`](total_milliseconds.md): the next unit up.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_nanoseconds.html">
    <span class="label">Previous</span>
    <span class="title">total_nanoseconds</span>
  </a>
  <a class="next-link" href="total_milliseconds.html">
    <span class="label">Next</span>
    <span class="title">total_milliseconds</span>
  </a>
</div>
