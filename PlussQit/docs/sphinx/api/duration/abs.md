<!-- pluss-hand-authored -->

# Duration::abs

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::abs() const;
Duration Duration::sign() const;
Duration Duration::clip(const Duration& lo, const Duration& hi) const;
```

## What It Does

`abs()` returns the magnitude of each element, discarding direction (negative
durations, built from e.g. `pluss::duration::minutes(-15)`, become positive).
`sign()` returns `-1`, `0`, or `1` per element (as a `Duration`, not a plain
integer `Series`), matching the numeric-sign convention. `clip(lo, hi)` bounds
every element into `[lo, hi]` — the two-sided cousin of clamping, taking two
`Duration` bounds rather than one.

## Parameters

**lo**, **hi** (`const Duration&`, `clip` only)

The inclusive lower and upper bound. Both broadcast against `self`.

## Returns

`Duration`
    All three return a `Duration`, not a `Series` — unlike the comparison
    family, these stay in duration-space.

## Examples

Example 1. `abs()` and `sign()` on a negative duration, and `sign()` on a
positive one for contrast.

```cpp
auto neg_fifteen_min = pluss::duration::minutes(-15);
auto ninety_min = pluss::duration::minutes(90);
std::cout << "abs=" << pluss::print_series(neg_fifteen_min.abs().total_minutes()) << "\n";
std::cout << "sign_neg=" << pluss::print_series(neg_fifteen_min.sign().total_nanoseconds()) << "\n";
std::cout << "sign_pos=" << pluss::print_series(ninety_min.sign().total_nanoseconds());
```

### Output

```text
abs=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=15.0000
sign_neg=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-1.0000
sign_pos=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.0000
```

`abs(-15 min) = 15 min`. `sign()`'s result is itself a `Duration` — one whose
magnitude is exactly `1` nanosecond, positive or negative — so reading it back
through `total_nanoseconds()` (not `total_days()` or another larger unit,
which would show a vanishingly small fraction) is the natural way to inspect
it directly.

Example 2. `clip` on an array, bounding it into `[2 days, 6 days]`.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
auto clipped = ranged.clip(pluss::duration::days(2), pluss::duration::days(6));
std::cout << pluss::print_series(clipped.total_days());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[2.0000, 2.0000, 4.0000, 6.0000, 6.0000, 6.0000]
```

`[0, 2, 4, 6, 8, 10]` days clipped into `[2, 6]`: `0`→`2` and `2`→`2` (floor),
`4` and `6` pass through, `8`→`6` and `10`→`6` (ceiling).

## Notes

- `sign()`'s output is itself a `Duration` (not an `int` `Series`) — read it
  back through a `total_*` conversion (or a comparison) rather than treating
  it as a plain number directly.
- `clip`'s bounds are both required `Duration` arguments — there is no
  one-sided "clip_min"/"clip_max" pair the way `Series` has
  [`clamp_min`](../series/clamp_min.md)/[`clamp_max`](../series/clamp_max.md).

## See Also

- [`eq`](eq.md): comparing durations, one way to check a sign or magnitude
  condition without calling `abs`/`sign` directly.
- [`arithmetic`](arithmetic.md): `mul`/`div`/`floordiv`/`mod`, the other
  numeric transforms on `Duration`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="total.html">
    <span class="label">Next</span>
    <span class="title">total</span>
  </a>
</div>
