<!-- pluss-hand-authored -->

# Series::isclose

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::isclose(const Series& other) const
Series Series::isclose(const Series& other, double rtol, double atol) const
Series Series::isclose(const Series& other, double rtol, double atol, bool equal_nan) const
```

## What It Does

Broadcasts the two operands and returns a `Bool` `Series` marking, per
position, whether the values agree **within a tolerance**.

For a receiver value `a` and a reference value `b`, the rule is:

```text
close  iff  a == b                              (exact equality wins first)
       or   |a - b| <= atol + rtol * |b|        (and both a, b finite)
```

Two consequences of that first line: exactly-equal finite values are always
close regardless of tolerance, and matching infinities (`+inf` vs `+inf`,
`-inf` vs `-inf`) are close — but opposite infinities are not, because
`|inf - (-inf)|` is not finite.

The tolerance has two terms because a single one never fits everywhere:
`rtol * |b|` scales with the magnitude being compared (good for large
numbers), and `atol` is a fixed floor that takes over near zero, where
`rtol * |b|` shrinks to almost nothing. The defaults are `rtol = 1e-5`,
`atol = 1e-8`.

Because `rtol` multiplies `|b|` — the **other** operand — the relation is
**not symmetric**. `a.isclose(b)` and `b.isclose(a)` can disagree near the
boundary. Choose the reference deliberately.

`isclose` gives you the *mask* — where the comparison fails. To reduce it to
one `bool`, use [`allclose`](allclose.md).

## Parameters

**other** (`const Series&`): the reference values. Must have the **same
dtype** and platform as `self`, and a broadcast-compatible shape. (This is
stricter than [`equal`](equal.md), which tolerates differing dtype.)

**rtol** (`double`): nonnegative relative tolerance, multiplied by
`|other|`.

**atol** (`double`): nonnegative absolute floor. Dominates near zero.

**equal_nan** (`bool`, default `false`): when `true`, positions where **both**
operands are `NaN` are marked close. A `NaN` is never close to a finite value
or to an infinity.

## Returns

`Series`
    A `Bool` `Series` of the broadcast shape, `true` where the pair is close.

## Raises

- if the dtypes differ, or the platforms differ
- if `rtol < 0` or `atol < 0`
- if the shapes are not broadcast-compatible

## Gradient

`isclose` is a predicate — its `Bool` result carries no useful derivative
across the comparison boundary.

## Examples

Example 1. Same shape, one value inside the default tolerance and one
outside.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({1.0 + 1e-9, 2.0 + 1e-3, 3.0});
pluss::series::print(std::cout, a.isclose(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]
```

Position `0` is off by `1e-9` — well inside `atol + rtol*|b| ≈ 1e-8 + 2e-5`.
Position `1` is off by `1e-3`, far outside. Position `2` is exactly equal.

Example 2. Relative tolerance is directional.

```cpp
std::cout << std::boolalpha
          << "9 close to 10: "
          << pluss::make_series({9.0}).isclose(pluss::make_series({10.0}), 0.1, 0.0).item<bool>() << "\n"
          << "10 close to 9: "
          << pluss::make_series({10.0}).isclose(pluss::make_series({9.0}), 0.1, 0.0).item<bool>() << "\n";
```

### Output

```text
9 close to 10: true
10 close to 9: false
```

The allowance in the first call is `0.1 * |10| = 1`, and the error is exactly
`1` — it passes. Reversed, the allowance is `0.1 * |9| = 0.9`, too small.

Example 3. Broadcasting turns `isclose` into a comparison grid — its main
use over `allclose` is seeing *where* things diverge.

```cpp
auto a = pluss::make_series({9.0, 20.0}).reshape({2, 1});
auto b = pluss::make_series({{10.0, 20.0, 40.0}});
pluss::series::print(std::cout, a.isclose(b, 0.1, 0.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡  true false false ⎤
⎣ false  true false ⎦
```

The `[2, 1]` receiver and `[1, 3]` reference broadcast to `[2, 3]`; each cell
is one `isclose` check.

Example 4. Near zero, `atol` carries the comparison — `rtol` alone vanishes.

```cpp
auto a = pluss::make_series({0.0, 0.0});
auto b = pluss::make_series({1e-9, 1e-3});
std::cout << "rtol only (atol=0): ";
pluss::series::print(std::cout, a.isclose(b, 1e-5, 0.0));
std::cout << "default atol=1e-8:  ";
pluss::series::print(std::cout, a.isclose(b));
```

### Output

```text
rtol only (atol=0): Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, false]default atol=1e-8:  Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

With `atol = 0` the allowance at `b = 1e-9` is `1e-5 * 1e-9 ≈ 1e-14` — even a
`1e-9` gap fails. The default `atol = 1e-8` lets the first pair through
(`1e-9 <= 1e-8`) while the `1e-3` gap still fails. This is why you almost
always want a nonzero `atol` when values can approach zero.

Example 5. Exact equality is checked first — matching infinities pass, and
`rtol = atol = 0` gives a pure exact comparison.

```cpp
double inf = std::numeric_limits<double>::infinity();
auto a = pluss::make_series({inf, -inf, 5.0});
auto b = pluss::make_series({inf, inf, 5.0});
std::cout << "matching +inf, opposite inf, equal finite: ";
pluss::series::print(std::cout, a.isclose(b));

auto x = pluss::make_series({1.0, 1.0000001});
auto y = pluss::make_series({1.0, 1.0});
std::cout << "rtol=atol=0 (pure exact): ";
pluss::series::print(std::cout, x.isclose(y, 0.0, 0.0));
```

### Output

```text
matching +inf, opposite inf, equal finite: Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]rtol=atol=0 (pure exact): Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

`+inf` vs `+inf` → `true` (exact). `+inf` vs `-inf` → `false`
(`|inf + inf|` is not finite). With both tolerances zero, `isclose` reduces
to elementwise exact equality — like [`eq`](eq.md), but still subject to the
dtype-match requirement.

Example 6. `NaN` and infinity, with and without `equal_nan`.

```cpp
double inf = std::numeric_limits<double>::infinity();
double nan = std::numeric_limits<double>::quiet_NaN();
auto a = pluss::make_series({inf, -inf, nan, 1.0});
auto b = pluss::make_series({inf, inf, nan, 1.000001});
pluss::series::print(std::cout, a.isclose(b)) << "\n";
pluss::series::print(std::cout, a.isclose(b, 1e-5, 1e-8, true));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, false, true]
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, true]
```

`equal_nan = true` flips only position `2` (both `NaN`); `-inf` vs `+inf`
stays `false`.

Example 7. Contract violations fail before any comparison.

```cpp
try {
  pluss::make_series({1.0}).isclose(pluss::make_series({1}));   // float vs int
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
try {
  pluss::make_series({1.0}).isclose(pluss::make_series({1.0}), -1.0, 0.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
isclose: receiver and other must have the same dtype
isclose: rtol must be greater than or equal to zero
```

## Gotchas And Failure Modes

- **Not symmetric.** `rtol` scales `|other|`, so `a.isclose(b)` and
  `b.isclose(a)` can differ near the boundary — pick the reference operand
  deliberately.
- **`atol` matters near zero.** With `atol = 0`, values close to zero almost
  never pass, because `rtol * |b|` collapses. Keep a nonzero `atol` for
  data that can be near zero.
- **Dtype must match exactly** — unlike [`equal`](equal.md). Convert one
  side with [`to`](to.md) first. Integer/`Bool` inputs are promoted to
  `Float32` internally for the arithmetic, but only after the dtype-equality
  check.
- Opposite infinities are never close; matching infinities always are.
- `NaN` is close only to a paired `NaN`, and only with `equal_nan = true`.

## Summary

`isclose(other, rtol = 1e-5, atol = 1e-8, equal_nan = false)` is the
per-position tolerant comparison:

- `close` iff exactly equal, or `|a - b| <= atol + rtol*|b|` with both
  finite
- exact equality (and matching infinities) always pass; opposite infinities
  never do
- asymmetric — `rtol` scales the **other** operand
- requires identical dtype (stricter than [`equal`](equal.md))
- reduce to one `bool` with [`allclose`](allclose.md)

## Choosing A Comparison

| You want | Method | Broadcasts | Tolerance |
|---|---|---|---|
| One exact yes/no | [`equal`](equal.md) | no | none |
| A per-element exact mask | [`eq`](eq.md) | yes | none |
| One approximate yes/no | [`allclose`](allclose.md) | yes | `rtol` / `atol` |
| A per-element approximate mask | `isclose` | yes | `rtol` / `atol` |

## See Also

- [`Series::allclose`](allclose.md): reduce this mask to one `bool`.
- [`Series::equal`](equal.md): exact whole-tensor comparison, no
  broadcasting, dtype-tolerant.
- [`Series::eq`](eq.md): exact elementwise comparison (`isclose` with both
  tolerances `0`, minus the dtype check nuance).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_signed.html">
    <span class="label">Previous</span>
    <span class="title">is_signed</span>
  </a>
  <a class="next-link" href="isfinite.html">
    <span class="label">Next</span>
    <span class="title">isfinite</span>
  </a>
</div>
