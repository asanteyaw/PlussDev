<!-- pluss-hand-authored -->

# Duration::total().nanoseconds()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::nanoseconds() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
nanoseconds — the finest-grained unit accessor, and the one every other
`total()` accessor is defined relative to (each just divides this value by a
different constant). Useful when you need the exact underlying tick count,
e.g. to inspect [`sign`](sign.md)'s output, which is always exactly `±1`
nanosecond in magnitude.

`total()` itself returns a small proxy object (`DurationTotal`) with one
method per unit — `.nanoseconds()`, `.microseconds()`, `.milliseconds()`,
`.seconds()`, `.minutes()`, `.hours()`, `.days()`, `.weeks()` — all
documented together on this page and its siblings. There is no flat
`Duration::total_nanoseconds()`/`total_seconds()`/etc.; `total()` is the sole
entry point.

## Parameters

None — both `total()` and `nanoseconds()` are zero-argument accessors.

## Returns

`DurationTotal` for `total()`
    A proxy object exposing the eight unit accessors.

`Series` for `nanoseconds()` (and every other unit method on the proxy)
    A floating-point `Series` at `self`'s shape, in the named unit.

## Examples

Example 1. A `90`-minute `Duration` read as raw nanoseconds.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::print_series(ninety_min.total().nanoseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5.4000e+12
```

`90` minutes is `5.4 * 10^12` nanoseconds.

## Notes

- `.total().nanoseconds()` never rounds or truncates — it is the exact
  underlying tick count, just as a floating-point `Series`.
- To go the other direction (a nanosecond count back into a `Duration`), use
  [`pluss::duration::nanos`](construction.md).
- `total()` returns a proxy holding its own copy of the `Duration` (cheap —
  `Duration` wraps a single reference-counted `Series`), not a reference, so
  it's safe to call directly on a temporary:
  `pluss::duration::minutes(90).total().seconds()`.
- Each unit method on the proxy computes independently on access — calling
  `.total()` itself does no conversion work; the actual `"total"` dispatch
  only runs when a specific unit method (`.nanoseconds()`, `.hours()`, etc.)
  is actually called.

## See Also

- [`total().microseconds()`](total_microseconds.md): the next unit up.
- [`construction`](construction.md): the inverse direction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="total_microseconds.html">
    <span class="label">Next</span>
    <span class="title">total_microseconds</span>
  </a>
</div>
