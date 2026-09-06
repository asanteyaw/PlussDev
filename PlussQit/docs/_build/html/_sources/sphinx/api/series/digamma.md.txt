<!-- pluss-hand-authored -->

# Series::digamma

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::digamma() const
Series& Series::digamma_()
```

## What It Does

Computes the digamma function elementwise. Digamma, written `psi`, is the
logarithmic derivative of the gamma function:

```
psi(x) = d/dx ln( Gamma(x) ) = Gamma'(x) / Gamma(x)
```

It is the function that [`lgamma()`](lgamma.md) differentiates to, so it turns
up in exactly the places `lgamma` does — the gradient of a beta function, the
score of a gamma or Dirichlet density, the derivative of a binomial coefficient
with respect to its arguments.

Anchor facts to carry:

- `psi(1) = -0.5772157`, the negative of the Euler-Mascheroni constant.
- `psi(2) = 0.4227843`.
- The recurrence `psi(x + 1) = psi(x) + 1/x` links consecutive arguments by a
  single reciprocal. It is the digamma image of `Gamma(x + 1) = x * Gamma(x)`.
- `psi(x)` grows like `ln(x)` for large `x`; it is negative on `(0, 1.4616...)`
  and positive after.
- At signed zero, the one-sided pole is preserved: `digamma(+0.0)` is `-inf`
  and `digamma(-0.0)` is `+inf`. Negative integer poles return `NaN`.
- On the intervals between the negative-integer poles `psi(x)` is finite. The
  value is recovered from the positive axis through the reflection identity
  `psi(1 - x) - psi(x) = pi / tan(pi x)`.

`digamma()` is an out-of-place unary transform: shape and platform are
preserved. Floating-point input keeps its dtype; integral input is promoted to
`float64`. `digamma_()` overwrites floating-point storage in place.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `psi(x)`. `digamma_()` returns a reference to the mutated current series.

An integral input is promoted to `float64`; a floating-point input keeps its
dtype. The in-place form requires floating-point storage because an integral
receiver cannot represent the result.

## Gradient

`digamma()` is differentiable. Its derivative is the trigamma function, which
is [`polygamma(1, x)`](polygamma.md):

```
d/dx psi(x) = psi'(x) = polygamma(1, x)
```

The backward pass evaluates trigamma directly with a recurrence-plus-asymptotic
routine, so the sensitivity is accurate across the whole positive axis and for
negative non-integers. Trigamma is strictly positive wherever it is defined, so
a tracked `digamma()` always contributes a positive local slope.

At zero and negative-integer poles, trigamma diverges to `+inf`. A reverse pass
through such an input therefore produces an infinite local sensitivity even
when the corresponding digamma value is `NaN` at a negative integer.

## Examples

Example 1. On the positive integers, consecutive values differ by a
reciprocal.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).digamma());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.5772, 0.4228, 0.9228, 1.2561]
```

`psi(2) = psi(1) + 1/1`, `psi(3) = psi(2) + 1/2`, `psi(4) = psi(3) + 1/3`.

Example 2. Half-integer arguments.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.5, 2.5}).digamma());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.9635, 0.0365, 0.7032]
```

`psi(0.5) = -gamma - 2 ln 2`, about `-1.9635`.

Example 3. The recurrence, checked directly. `psi(x + 1) - psi(x)` equals
`1/x`.

```cpp
auto x = pluss::make_series({2.0, 3.0, 7.0});
pluss::series::print(std::cout, x.add(1.0).digamma().sub(x.digamma()));
pluss::series::print(std::cout, x.reciprocal());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5000, 0.3333, 0.1429]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5000, 0.3333, 0.1429]
```

Example 4. Signed zero records which side approaches the pole. Negative integer
poles are `NaN`, while negative non-integers remain finite through reflection.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, -0.0, -1.0, -0.5}).digamma());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-inf, inf, nan, 0.0365]
```

Example 5. The backward pass is trigamma. With the input tracked, a reverse
pass fills the sensitivity with `polygamma(1, x)` evaluated at each element.

```cpp
auto x = pluss::make_series({1.5, 2.5, 3.5});
x.tracks_slope_(true);
x.digamma().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
pluss::series::print(std::cout, pluss::make_series({1.5, 2.5, 3.5}).polygamma(1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.9348, 0.4904, 0.3304]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.9348, 0.4904, 0.3304]
```

The sensitivity matches `polygamma(1, x)` term for term.

Example 6. Integral inputs are promoted to `float64` rather than rejected.

```cpp
pluss::series::print(std::cout, pluss::make_series({1, 2, 3}).digamma());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5772, 0.4228, 0.9228]
```

Example 7. `digamma_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
a.digamma_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5772, 0.4228, 0.9228]
```

## Gotchas And Failure Modes

- Signed zero returns a signed infinity: `+0.0` maps to `-inf`, while `-0.0`
  maps to `+inf`. Negative integer poles return `NaN`.
- The gradient is trigamma and diverges to `+inf` at those poles.
- For the derivatives of `ln Gamma` beyond the first, use
  [`polygamma(n, x)`](polygamma.md); `digamma(x)` is the `n = 0` case.
- An integral series is promoted to `float64`; the result is never integral.

## Summary

`digamma()` is `psi(x) = d/dx ln Gamma(x)` as a shape-preserving unary
transform:

- `psi(1) = -gamma`, `psi(2) = 1 - gamma`, and `psi(x + 1) = psi(x) + 1/x`
- signed infinities at zero, `NaN` at negative integers, and finite values at
  negative non-integers
- differentiable, with backward [`polygamma(1, x)`](polygamma.md); the gradient
  diverges at the poles
- integral inputs promote to `float64`
- `digamma_()` is the in-place form

## See Also

- [`Series::lgamma`](lgamma.md): the function digamma is the derivative of.
- [`Series::polygamma`](polygamma.md): the higher derivatives of `ln Gamma`;
  `digamma` is `polygamma(0, .)` and its gradient is `polygamma(1, .)`.
- [`Series::mvlgamma`](mvlgamma.md): the multivariate log-gamma, whose gradient
  is a sum of digamma terms.
- [`Series::beta`](beta.md): its log gradient is a difference of digamma values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="dim.html">
    <span class="label">Next</span>
    <span class="title">dim</span>
  </a>
</div>
