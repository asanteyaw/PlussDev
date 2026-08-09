<!-- pluss-hand-authored -->

# Duration::total().seconds()

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
DurationTotal Duration::total() const;
Series DurationTotal::seconds() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
seconds — [`total().nanoseconds()`](total_nanoseconds.md) divided by `1e9`.
This is the way out of duration-space back into an ordinary numeric `Series`,
e.g. for storing elapsed time in a DataFrame column (recall `Duration` itself
isn't a `ThreadKind` payload — see the
[API reference overview](../../duration_api_reference)) or feeding it into
arithmetic that doesn't care about the duration/instant distinction. See
[`total().nanoseconds()`](total_nanoseconds.md) for the full explanation of
the `total()` accessor.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in seconds.

## Examples

Example 1. A `90`-minute `Duration` read as seconds.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << pluss::print_series(ninety_min.total().seconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5400.0000
```

`90` minutes is `5400` seconds.

## Notes

- `.total().seconds()` never rounds or truncates — the result is always the
  exact (floating-point) division.
- To go the other direction (a number of seconds back into a `Duration`), use
  [`pluss::duration::seconds`](construction.md).

## See Also

- [`total().milliseconds()`](total_milliseconds.md): the finer unit this divides.
- [`total().minutes()`](total_minutes.md): the next unit up.
- [`construction`](construction.md): the inverse direction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_milliseconds.html">
    <span class="label">Previous</span>
    <span class="title">total_milliseconds</span>
  </a>
  <a class="next-link" href="total_minutes.html">
    <span class="label">Next</span>
    <span class="title">total_minutes</span>
  </a>
</div>
