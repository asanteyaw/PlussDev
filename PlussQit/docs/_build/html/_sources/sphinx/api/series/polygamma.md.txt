<!-- pluss-hand-authored -->

# Series::polygamma

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::polygamma(std::int64_t n) const
Series& Series::polygamma_(std::int64_t n)
Series& Series::polygamma_(std::int64_t n) const
```

## What It Does

Computes the polygamma function of order `n` elementwise. Polygamma is the
`n`-th derivative of the digamma function, equivalently the `(n + 1)`-th
derivative of `ln Gamma`:

```
polygamma(n, x) = d^n/dx^n psi(x) = d^(n+1)/dx^(n+1) ln( Gamma(x) )
```

The low orders have their own names:

- `polygamma(0, x)` is [`digamma(x)`](digamma.md).
- `polygamma(1, x)` is the trigamma function — the one that appears as the
  gradient of `digamma`, and as the Fisher information of several one-parameter
  families.
- `polygamma(2, x)` is tetragamma, and so on.

For `n >= 1` the closed form is a Hurwitz zeta value:

```
polygamma(n, x) = (-1)^(n+1) * n! * zeta(n + 1, x)
```

so `polygamma` alternates in sign with `n`: order `1` is positive, order `2` is
negative, order `3` positive. The forward pass uses that identity with an
Euler-Maclaurin evaluation of [`zeta`](zeta.md), accurate to full precision in
a handful of terms.

Anchor facts:

- `polygamma(1, 1) = pi^2 / 6 = 1.6449341`.
- `polygamma(1, x + 1) = polygamma(1, x) - 1/x^2`, the derivative of the digamma
  recurrence.
- `polygamma(2, 1) = -2 * zeta(3) = -2.4041138`.
- Every order has poles at `0` and the negative integers. For `n >= 1`, the
  implementation follows the Hurwitz-zeta continuation used by the established
  tensor convention: those poles are represented by signed infinity, while
  negative non-integers can have finite values.

`polygamma()` is out-of-place; `polygamma_()` overwrites the current storage.

## Parameters

**n** (`std::int64_t`): derivative order. Must be `>= 0`. `n = 0` routes to
[`digamma`](digamma.md).

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `polygamma(n, x)`. `polygamma_()` returns a reference to the mutated series.

An integral input is promoted to `float64`; floating-point input keeps its
dtype. The in-place form requires floating-point storage.

## Gradient

`polygamma(n, .)` is differentiable with respect to `x`. Its derivative is the
next order:

```
d/dx polygamma(n, x) = polygamma(n + 1, x)
```

The backward pass evaluates that higher order the same way the forward pass
does. `n` is a structural integer argument and carries no gradient. At a pole,
the value or sensitivity can be infinite; near a pole, expect very large
magnitudes.

## In-Place Form

`polygamma_()` is the in-place sibling of [`polygamma()`](polygamma.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `polygamma_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0});
x.polygamma_(0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.57721566, 0.42278434]
```

## Examples

Example 1. Order `0` is digamma.

```cpp
pluss::series::print(std::cout, pluss::make_series({1.0, 2.0, 3.0}).polygamma(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5772, 0.4228, 0.9228]
```

Example 2. Order `1` is trigamma. At `x = 1` the value is `pi^2 / 6`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0}).polygamma(1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.6449, 0.6449, 0.3949]
```

Successive entries drop by `1/x^2`: `1.6449341 - 1 = 0.6449341`, then
`0.6449341 - 1/4 = 0.3949341`.

Example 3. Order `2` at `x = 1` is `-2 * zeta(3)`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0}).polygamma(2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-2.4041, -0.4041]
```

Example 4. Negative non-integers are not rejected. Their value follows the same
continuation identity; here the trigamma recurrence crosses the pole at zero.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-0.5, 0.5, 1.5}).polygamma(1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8.9348, 4.9348, 0.9348]
```

Example 5. The backward pass of order `n` is order `n + 1`.

```cpp
auto x = pluss::make_series({1.5, 2.5});
x.tracks_slope_(true);
x.polygamma(1).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
pluss::series::print(std::cout, pluss::make_series({1.5, 2.5}).polygamma(2));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.8288, -0.2362]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.8288, -0.2362]
```

Example 6. Poles at `0` and the negative integers diverge.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, -1.0, 2.0}).polygamma(1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[inf, inf, 0.6449]
```

Example 7. A negative order is rejected.

```cpp
try {
  std::cout << pluss::make_series({1.0}).polygamma(-1);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
polygamma: n must be >= 0
```

## Gotchas And Failure Modes

- `n` must be `>= 0`. A negative order raises `polygamma: n must be >= 0`.
- `n = 0` is digamma, not a constant; do not expect `polygamma(0, x)` to be `1`.
- For `n >= 1` the sign alternates: odd orders are positive on the positive
  axis, even orders negative (the `(-1)^(n+1)` factor). `n = 0` (digamma) is not
  sign-definite.
- Poles at `0` and the negative integers diverge. Negative non-integers are
  valid continuation points and can return finite values.
- Integral input promotes out of place, but `polygamma_()` requires a
  floating-point receiver.
- `n` carries no gradient; only `x` does.

## Summary

`polygamma(n, x)` is the `n`-th derivative of `digamma`:

- `polygamma(0, .)` is [`digamma`](digamma.md); `polygamma(1, .)` is trigamma
- for `n >= 1`, equals `(-1)^(n+1) * n! * zeta(n + 1, x)`
- sign alternates with `n` on the positive axis; poles diverge, while negative
  non-integers are continued
- differentiable in `x` with backward `polygamma(n + 1, x)`; `n` is structural
- `polygamma_()` is the in-place form

## See Also

- [`Series::digamma`](digamma.md): the `n = 0` case and the function polygamma
  differentiates.
- [`Series::zeta`](zeta.md): the Hurwitz zeta the closed form is built from.
- [`Series::lgamma`](lgamma.md): `polygamma(n, .)` is its `(n + 1)`-th
  derivative.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="platform.html">
    <span class="label">Previous</span>
    <span class="title">platform</span>
  </a>
  <a class="next-link" href="positive.html">
    <span class="label">Next</span>
    <span class="title">positive</span>
  </a>
</div>
