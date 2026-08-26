<!-- pluss-hand-authored -->

# Duration::total().days()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::days() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
days — [`total().nanoseconds()`](total_nanoseconds.md) divided by
`86'400 * 1e9`. See [`total().nanoseconds()`](total_nanoseconds.md) for the
full explanation of the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in days.

## Examples

Example 1. `.total().days()` over an array `Duration`.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
std::cout << pluss::series::print(ranged.total().days());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 2.0000, 4.0000, 6.0000, 8.0000, 10.0000]
```

The array shape carries through the conversion — one converted value per
element, same as every other unit accessor in this family.

## Notes

- `.total().days()` never rounds or truncates — the result is always the
  exact (floating-point) division.

## See Also

- [`total().hours()`](total_hours.md): the finer unit this divides.
- [`total().weeks()`](total_weeks.md): the next unit up.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_hours.html">
    <span class="label">Previous</span>
    <span class="title">total_hours</span>
  </a>
  <a class="next-link" href="total_weeks.html">
    <span class="label">Next</span>
    <span class="title">total_weeks</span>
  </a>
</div>
