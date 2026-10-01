<!-- pluss-hand-authored -->

# Duration::total().milliseconds()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::milliseconds() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
milliseconds — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`1'000'000`. See [`total().nanoseconds()`](total_nanoseconds.md) for the full
explanation of the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in milliseconds.

## Examples

Example 1. A `90`-minute `Duration` read as milliseconds.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::series::print(ninety_min.total().milliseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.4000e+06
```

`90` minutes is `5.4 * 10^6` milliseconds.

## Notes

- `.total().milliseconds()` never rounds or truncates — the result is always
  the exact (floating-point) division.

## See Also

- [`total().microseconds()`](total_microseconds.md): the finer unit this divides.
- [`total().seconds()`](total_seconds.md): the next unit up.
