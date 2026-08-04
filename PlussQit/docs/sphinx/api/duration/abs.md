<!-- pluss-hand-authored -->

# Duration::abs

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::abs() const;
```

## What It Does

Returns the magnitude of each element, discarding direction — negative
durations, built from e.g. `pluss::duration::minutes(-15)`, become positive.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Duration`
    A new duration holding the elementwise absolute value.

## Examples

Example 1. `abs()` on a negative duration.

```cpp
auto neg_fifteen_min = pluss::duration::minutes(-15);
std::cout << pluss::duration::print(neg_fifteen_min.abs());
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 00:15:00.000000000
```

`abs(-15 min) = 15 min`, with the leading `-` gone from the printed form.

## Notes

- `abs()` returns a `Duration`, not a `Series` — unlike the comparison family,
  it stays in duration-space.
- `abs(d)` is equivalent to `d if d.ge(pluss::duration::nanos(0)) else -d`,
  but computed directly.

## See Also

- [`sign`](sign.md): direction alone, the complementary piece of information
  `abs` discards.
- [`clip`](clip.md): bounding into a range, another unary transform.
- [`neg`](neg.md): unary negation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="sign.html">
    <span class="label">Next</span>
    <span class="title">sign</span>
  </a>
</div>
