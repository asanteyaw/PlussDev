<!-- pluss-hand-authored -->

# Duration::clip

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::clip(const Duration& lo, const Duration& hi) const;
```

## What It Does

Bounds every element into `[lo, hi]` — the two-sided cousin of clamping,
taking two `Duration` bounds rather than one.

## Parameters

**lo**, **hi** (`const Duration&`)

The inclusive lower and upper bound. Both broadcast against `self`.

## Returns

`Duration`
    A new duration with every element bounded into `[lo, hi]`.

## Examples

Example 1. Clip an array, bounding it into `[2 days, 6 days]`.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
auto clipped = ranged.clip(pluss::duration::days(2), pluss::duration::days(6));
std::cout << pluss::duration::print(clipped);
```

### Output

```text
Duration(shape=[6], dim=1, numel=6)
[ 2 days 00:00:00.000000000, 2 days 00:00:00.000000000, 4 days 00:00:00.000000000, 6 days 00:00:00.000000000, 6 days 00:00:00.000000000, 6 days 00:00:00.000000000 ]
```

`[0, 2, 4, 6, 8, 10]` days clipped into `[2, 6]`: `0`→`2` and `2`→`2` (floor),
`4` and `6` pass through, `8`→`6` and `10`→`6` (ceiling).

## Notes

- `clip`'s bounds are both required `Duration` arguments — there is no
  one-sided "clip_min"/"clip_max" pair the way `Series` has
  [`clamp_min`](../series/clamp_min.md)/[`clamp_max`](../series/clamp_max.md).

## See Also

- [`abs`](abs.md): magnitude alone, another unary transform.
- [`sign`](sign.md): direction alone, another unary transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sign.html">
    <span class="label">Previous</span>
    <span class="title">sign</span>
  </a>
  <a class="next-link" href="total_nanoseconds.html">
    <span class="label">Next</span>
    <span class="title">total_nanoseconds</span>
  </a>
</div>
