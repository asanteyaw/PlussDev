<!-- pluss-hand-authored -->

# Duration::sign

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::sign() const;
```

## What It Does

Returns `-1`, `0`, or `1` per element (as a `Duration`, not a plain integer
`Series`), matching the numeric-sign convention — the complementary piece of
information to [`abs`](abs.md), which discards direction and keeps magnitude.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Duration`
    A duration whose magnitude is exactly `1` nanosecond, positive, negative,
    or zero, matching `self`'s sign per element.

## Examples

Example 1. `sign()` on a negative duration and a positive one, for contrast.

```cpp
auto neg_fifteen_min = pluss::duration::minutes(-15);
auto ninety_min = pluss::duration::minutes(90);
std::cout << "sign_neg=" << pluss::series::print(neg_fifteen_min.sign().total().nanoseconds()) << "\n";
std::cout << "sign_pos=" << pluss::series::print(ninety_min.sign().total().nanoseconds());
```

### Output

```text
sign_neg=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-1.0000
sign_pos=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.0000
```

`sign()`'s result is itself a `Duration` — one whose magnitude is exactly `1`
nanosecond, positive or negative — so reading it back through
`.total().nanoseconds()` (not `.total().days()` or another larger unit,
which would show a vanishingly small fraction) is the natural way to inspect
it directly.

## Notes

- `sign()`'s output is itself a `Duration` (not an `int` `Series`) — read it
  back through a `.total()` conversion (or a comparison) rather than treating
  it as a plain number directly.
- `pluss::duration::print(neg_fifteen_min.sign())` also works, and prints
  `-0 days 00:00:00.000000001` — accurate, but harder to read at a glance
  than `.total().nanoseconds()`'s plain `-1.0000` for a value whose magnitude
  is always exactly one nanosecond.

## See Also

- [`abs`](abs.md): magnitude alone, the complementary piece of information
  `sign` discards.
- [`clip`](clip.md): bounding into a range, another unary transform.
