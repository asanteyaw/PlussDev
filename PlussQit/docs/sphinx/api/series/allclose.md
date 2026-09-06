<!-- pluss-hand-authored -->

# Series::allclose

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::allclose(const Series& other,
                      double rtol = 1e-5,
                      double atol = 1e-8,
                      bool equal_nan = false) const
```

## What It Does

Returns one C++ `bool`: `true` when **every** broadcasted pair of values
satisfies the [`isclose`](isclose.md) rule.

It is exactly `self.isclose(other, rtol, atol, equal_nan).all().item<bool>()`
— the convenient reduction for a test assertion or a guard clause, where you
want a single yes/no rather than a mask. When a comparison fails and you need
to know *which* positions, drop down to [`isclose`](isclose.md).

The per-position criterion, for receiver value `a` and reference value `b`:

```text
close  iff  a == b                              (exact equality wins first)
       or   |a - b| <= atol + rtol * |b|        (and both a, b finite)
```

So exactly-equal finite values and matching infinities always pass; opposite
infinities never do; and paired `NaN`s pass only with `equal_nan = true`.
Every position must pass for `allclose` to return `true` — one failure
anywhere is `false`.

Because `rtol` scales `|b|` — the **other** operand — `allclose` is **not
symmetric**. `a.allclose(b)` and `b.allclose(a)` can disagree near the
tolerance boundary.

Unlike [`equal`](equal.md), `allclose` requires the two operands to have the
**same dtype** (it inherits `isclose`'s contract). It does, however,
broadcast — so it can compare differently shaped operands, where `equal`
would simply say `false`.

## Parameters

**other** (`const Series&`): the reference values. Same dtype and platform
as `self`; broadcast-compatible shape.

**rtol** (`double`, default `1e-5`): nonnegative relative tolerance,
multiplied by `|other|`.

**atol** (`double`, default `1e-8`): nonnegative absolute floor. Dominates
near zero.

**equal_nan** (`bool`, default `false`): when `true`, positions where both
operands are `NaN` count as close.

## Returns

`bool`
    `true` iff every broadcasted pair is close.

## Raises

- if the dtypes differ, or the platforms differ
- if `rtol < 0` or `atol < 0`
- if the shapes are not broadcast-compatible

## Gradient

Not applicable — `allclose()` returns a plain `bool`.

## Examples

Example 1. A basic pass and a basic fail.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
std::cout << std::boolalpha
          << "within tol:  " << a.allclose(pluss::make_series({1.0 + 1e-9, 2.0, 3.0})) << "\n"
          << "one far off: " << a.allclose(pluss::make_series({1.0, 2.5, 3.0})) << "\n";
```

### Output

```text
within tol:  true
one far off: false
```

One position out of tolerance is enough to make the whole call `false`.

Example 2. Swapping the operands can flip the answer near the boundary,
because `rtol` scales the reference.

```cpp
std::cout << std::boolalpha
          << "a=9,  b=10: " << pluss::make_series({9.0}).allclose(pluss::make_series({10.0}), 0.1, 0.0) << "\n"
          << "a=10, b=9:  " << pluss::make_series({10.0}).allclose(pluss::make_series({9.0}), 0.1, 0.0) << "\n";
```

### Output

```text
a=9,  b=10: true
a=10, b=9:  false
```

Allowance `0.1 * 10 = 1` (error `1`, passes) versus `0.1 * 9 = 0.9` (error
`1`, fails). If a symmetric notion of closeness matters, pick one operand as
the canonical reference and always pass it in the same position.

Example 3. `equal_nan` repairs paired `NaN`s, but every other position must
still pass on its own.

```cpp
double inf = std::numeric_limits<double>::infinity();
double nan = std::numeric_limits<double>::quiet_NaN();
auto a = pluss::make_series({inf, -inf, nan, 1.0});
auto b = pluss::make_series({inf, inf, nan, 1.000001});
std::cout << std::boolalpha
          << "NaNs unequal: " << a.allclose(b) << "\n"
          << "NaNs equal:   " << a.allclose(b, 1e-5, 1e-8, true) << "\n";
```

### Output

```text
NaNs unequal: false
NaNs equal:   false
```

Even with `equal_nan = true`, `-inf` at position `1` does not match `+inf`,
so the whole call is still `false`. `equal_nan` only fixes the `NaN`
position, not the rest.

Example 4. An invalid tolerance is rejected by the shared `isclose`
contract — note the diagnostic names `isclose`.

```cpp
try {
  pluss::make_series({1.0}).allclose(pluss::make_series({1.0}), -1.0, 0.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
isclose: rtol must be greater than or equal to zero
```

Example 5. `allclose` requires matching dtype — where [`equal`](equal.md)
does not.

```cpp
try {
  pluss::make_series({1.0, 2.0}).allclose(pluss::make_series({1, 2}));   // float vs int
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
std::cout << "equal() with the same pair: " << std::boolalpha
          << pluss::make_series({1.0, 2.0}).equal(pluss::make_series({1, 2})) << "\n";
```

### Output

```text
isclose: receiver and other must have the same dtype
equal() with the same pair: true
```

Convert first (`other.to(self.dtype())`) when the dtypes genuinely differ but
you want a tolerant comparison.

## Gotchas And Failure Modes

- **Not symmetric** — `rtol` scales `|other|`. Fix the reference operand's
  position if closeness needs to be order-independent.
- **`atol` matters near zero.** With `atol = 0`, values near zero almost
  never pass. Keep the nonzero default (or larger) for near-zero data.
- **Dtype must match** — unlike [`equal`](equal.md). Convert with
  [`to`](to.md) first.
- Broadcasting can compare differently shaped operands; if shape equality is
  part of the requirement, gate with [`is_same_size`](is_same_size.md) or use
  [`equal`](equal.md).
- One failing position anywhere makes the whole call `false` — use
  [`isclose`](isclose.md) to find it.

## Summary

`allclose(other, rtol = 1e-5, atol = 1e-8, equal_nan = false)` reduces
[`isclose`](isclose.md) to a single `bool`:

- `true` iff every broadcasted pair is close (exact-equal, or within
  `atol + rtol*|b|`)
- asymmetric; `atol` carries the comparison near zero
- broadcasts, but requires identical dtype (stricter than
  [`equal`](equal.md))
- drop to [`isclose`](isclose.md) for the per-position mask

## Choosing A Comparison

| You want | Method | Broadcasts | Tolerance |
|---|---|---|---|
| One exact yes/no | [`equal`](equal.md) | no | none |
| A per-element exact mask | [`eq`](eq.md) | yes | none |
| One approximate yes/no | `allclose` | yes | `rtol` / `atol` |
| A per-element approximate mask | [`isclose`](isclose.md) | yes | `rtol` / `atol` |

## See Also

- [`Series::isclose`](isclose.md): the per-position mask, the full formula,
  and more broadcasting detail.
- [`Series::equal`](equal.md): exact whole-tensor comparison — no tolerance,
  no broadcasting, dtype-tolerant.
- [`Series::all`](all.md): the reduction `allclose` applies to `isclose`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="amax.html">
    <span class="label">Next</span>
    <span class="title">amax</span>
  </a>
</div>
