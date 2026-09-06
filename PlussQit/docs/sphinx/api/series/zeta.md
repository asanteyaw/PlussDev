<!-- pluss-hand-authored -->

# Series::zeta

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::zeta(double other) const
```

## What It Does

Computes the Hurwitz zeta function elementwise. `self` supplies the exponent
`x`; the argument supplies the offset `q`:

```
zeta(x, q) = sum_{k>=0} (q + k)^(-x)
```

With `q = 1` this is the Riemann zeta function, `zeta(x) = sum_{n>=1} n^(-x)`.
The Hurwitz form generalizes the sum to start from `q` instead of `1`, and it
is the piece the polygamma functions are built from:

```
polygamma(n, q) = (-1)^(n+1) * n! * zeta(n + 1, q)      for n >= 1
```

So [`polygamma`](polygamma.md) — and therefore the gradient of
[`digamma`](digamma.md) — routes through this function.

The forward pass uses Euler-Maclaurin acceleration: a short direct sum takes
the offset past `~9`, then an asymptotic correction with Bernoulli
coefficients converges to full `float64` precision in a handful of terms.

Domain:

- `x = 1` is the harmonic pole; `zeta(1, q)` returns `+inf`.
- `x < 1` returns `NaN`; this method does not provide analytic continuation
  in the exponent.
- Positive `q` is the ordinary Hurwitz-zeta domain.
- A non-positive integer `q` places one term of the series on a pole and
  returns `+inf`.
- A negative, non-integral `q` is supported when `x` is an integer. For a
  non-integral exponent, raising the negative leading terms to `-x` would
  leave the real number system, so the real-valued method returns `NaN`.

## Parameters

**other** (`double`): the offset `q`, a scalar constant applied to every
element of `self`.

## Returns

`Series`
    A `Series` with the same shape as `self`, each element `zeta(x, q)`, or
    `+inf` / `NaN` on the domain boundary.

An integral input is promoted to `float64`.

## Gradient

The method form is **not differentiable**. `self` is the exponent `x`, whose
derivative — `sum_k -ln(q + k) (q + k)^(-x)` — has no routing here and matches
the `not_implemented` status the exponent slot has elsewhere. `q` arrives as a
plain `double`, so it is a constant.

An objective that reaches its input only through `zeta` is therefore
disconnected: if `self` tracks slope, calling [`reverse()`](reverse.md) on it
raises (Example 5). The offset derivative that *does* have a closed form,
`d/dq zeta(x, q) = -x * zeta(x + 1, q)`, applies only when `q` is supplied as a
differentiable series through the lower-level routing layer, not through this
method.

## Examples

Example 1. With `q = 1` this is the Riemann zeta. `zeta(2) = pi^2/6`,
`zeta(4) = pi^4/90`, `zeta(6) = pi^6/945`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 4.0, 6.0}).zeta(1.0)
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.6449, 1.0823, 1.0173]
```

Example 2. The Hurwitz offset. `zeta(2, 1)`, `zeta(2, 2)`, `zeta(2, 3)` each
drop the leading terms of the Riemann sum.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 2.0, 2.0}).zeta(1.0)
) << "\n";
pluss::series::print(std::cout, pluss::make_series({2.0}).zeta(2.0)) << "\n";
pluss::series::print(std::cout, pluss::make_series({2.0}).zeta(3.0)) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.6449, 1.6449, 1.6449]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.6449]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.3949]
```

`zeta(2, 2) = zeta(2) - 1 = 0.6449341`, and `zeta(2, 3) = zeta(2) - 1 - 1/4 =
0.3949341`.

Example 3. The recurrence `zeta(x, q) - zeta(x, q + 1) = q^(-x)`.

```cpp
auto exponents = pluss::make_series({2.0, 3.0});
auto dropped_term = exponents.zeta(1.0).sub(exponents.zeta(2.0));
pluss::series::print(std::cout, dropped_term) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 1.0000]
```

Here `q = 1`, so the removed leading term is `1^(-x) = 1` for both
exponents. This recurrence is useful when shifting an awkward offset into a
more convenient range.

Example 4. A negative fractional offset distinguishes three cases. Integral
exponents remain real, a fractional exponent does not, and a non-positive
integer offset is a pole.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 3.0, 2.5}).zeta(-0.5)
) << "\n";
pluss::series::print(std::cout, pluss::make_series({2.0}).zeta(0.0)) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8.9348, 0.4144, nan]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[inf]
```

Example 5. The method form is non-differentiable. Tracking the exponent does
not make the result track slope because this public overload has no exponent
backward rule.

```cpp
auto s = pluss::make_series({2.0, 3.0});
s.tracks_slope_(true);
auto obj = s.zeta(1.0).sum();
std::cout << "exponent tracks: " << s.tracks_slope() << "\n";
std::cout << "objective tracks: " << obj.tracks_slope() << "\n";
```

### Output

```text
exponent tracks: 1
objective tracks: 0
```

## Gotchas And Failure Modes

- Argument order: `self` is the exponent `x`, the argument is the offset `q`.
  `x.zeta(1.0)` is the ordinary Riemann zeta of `x`.
- `x` must be strictly greater than `1`. `x = 1` returns `+inf`; `x < 1`
  returns `NaN`. This routine does not do analytic continuation into the
  critical strip.
- Negative fractional `q` is real only for integral `x`. A non-positive
  integer `q` is a pole and returns `+inf`.
- The method form carries no gradient. Do not place a tracked series in the
  exponent and expect a sensitivity — `reverse()` will raise.

## Summary

`zeta(q)` is the Hurwitz zeta `sum_{k>=0} (q + k)^(-x)`, with the exponent `x`
from `self` and the offset `q` from the argument:

- `zeta(1.0)` is the Riemann zeta; `zeta(2) = pi^2/6`
- the closed form behind [`polygamma`](polygamma.md) for `n >= 1`
- requires `x > 1`; positive `q` is the ordinary domain, while negative `q`
  follows the real-valued integral-exponent rules described above
- the method form is non-differentiable

## See Also

- [`Series::polygamma`](polygamma.md): built from `zeta(n + 1, x)`.
- [`Series::digamma`](digamma.md): its gradient routes through this function.
- [`Series::lgamma`](lgamma.md): the other core gamma-family special function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="xlogy.html">
    <span class="label">Previous</span>
    <span class="title">xlogy</span>
  </a>
</div>
