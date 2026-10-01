<!-- pluss-hand-authored -->

# Duration::total().weeks()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::weeks() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
weeks — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`7 * 86'400 * 1e9`. The coarsest unit accessor in this family. See
[`total().nanoseconds()`](total_nanoseconds.md) for the full explanation of
the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in weeks.

## Examples

Example 1. A `21`-day `Duration` read as weeks.

```cpp
auto three_weeks = pluss::duration::days(21);
std::cout << pluss::series::print(three_weeks.total().weeks());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.0000
```

`21` days is exactly `3` weeks.

## Notes

- `.total().weeks()` never rounds or truncates — a duration that isn't an
  exact multiple of a week reads back as a non-integer number of weeks, same
  as every other unit accessor in this family.

## See Also

- [`total().days()`](total_days.md): the finer unit this divides.
- [`total().nanoseconds()`](total_nanoseconds.md): the finest unit, and the
  value every accessor in this family is ultimately derived from.
