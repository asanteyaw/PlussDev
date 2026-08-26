<!-- pluss-hand-authored -->

# Duration::where

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::where(const Series& mask, const Duration& other) const;
```

## What It Does

Elementwise conditional selection: **keeps `self`'s own value where `mask` is
`true`, and replaces it with the corresponding position of `other` where
`mask` is `false`** — the same "true keeps self, false replaces" convention
pandas' `Series.where(cond, other)` uses. This is the opposite reading from
`numpy.where(cond, x, y)`, which picks its *first* value argument where the
condition is true; `Duration::where` has no "first value" argument at all —
only `self` (implicit) and `other`.

```{important} Verify this direction before relying on it
This convention is easy to get backwards when writing the mask. The example
below is deliberately explicit about which positions end up as `self` and
which end up as `other`, so you can check your own mask against it before
trusting a `where` call that isn't obviously symmetric.
```

## Parameters

**mask** (`const Series&`)

A boolean mask, broadcast against `self`. Where `true`, `self`'s own value is
kept; where `false`, the corresponding position of `other` is used.

**other** (`const Duration&`)

The replacement source for masked-`false` positions.

## Returns

`Duration`
    A new duration: `self` where `mask` is `true`, `other` where `mask` is
    `false`.

## Examples

Example 1. Replace every position past `4` days with a `99`-day sentinel.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
auto mask = ranged.gt(pluss::duration::days(4));
std::cout << "mask=" << pluss::series::print(mask) << "\n";

auto replacement = pluss::duration::full({6}, pluss::duration::days(99));
auto picked = ranged.where(mask, replacement);
std::cout << "where=" << pluss::series::print(picked.total().days());
```

### Output

```text
mask=Series(shape=[6], strides=[1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[false, false, false, true, true, true]
where=Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[99.0000, 99.0000, 99.0000, 6.0000, 8.0000, 10.0000]
```

`ranged` is `[0, 2, 4, 6, 8, 10]` days; `mask` (`ranged > 4 days`) is
`[false, false, false, true, true, true]`. At the three `false` positions
(`0`, `2`, `4`), the result is `other`'s value (`99`). At the three `true`
positions (`6`, `8`, `10`), the result is `self`'s own original value,
unchanged.

## Notes

- `mask`'s "`true` keeps `self`, `false` selects `other`" direction is the
  same one pandas' `Series.where` uses, and the opposite of `numpy.where`'s
  "condition selects the first value" reading — worth double-checking against
  a small example like the one above if you're coming from `numpy`.
- [`fill_nat`](fill_nat.md) is the special case of `where` where `mask` is
  `isnat()` and `other` is the fill value.

## See Also

- [`fill_nat`](fill_nat.md): the missing-value-specific special case of this
  operation.
- [`isnat`](isnat.md), [`isvalid`](isvalid.md): common sources for `mask`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_nat.html">
    <span class="label">Previous</span>
    <span class="title">fill_nat</span>
  </a>
</div>
