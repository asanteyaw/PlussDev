<!-- pluss-hand-authored -->

# Series::mvlgamma

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::mvlgamma(std::int64_t p) const
Series& Series::mvlgamma_(std::int64_t p)
Series& Series::mvlgamma_(std::int64_t p) const
```

## What It Does

Computes the natural log of the multivariate gamma function of dimension `p`,
elementwise. The multivariate gamma `Gamma_p` is the normalizing constant that
shows up whenever a density is defined over `p x p` positive-definite matrices —
the Wishart and inverse-Wishart distributions, the matrix-variate `t`, the
marginal likelihood of a Gaussian with a conjugate covariance prior. It factors
into ordinary gamma functions:

```
Gamma_p(x) = pi^(p(p-1)/4) * prod_{j=1..p} Gamma( x + (1 - j)/2 )
```

and `mvlgamma()` returns the log of that:

```
mvlgamma(x, p) = (p(p-1)/4) * ln(pi) + sum_{j=1..p} lgamma( x + (1 - j)/2 )
```

Working in log space is the point, for the same reason it is with
[`lgamma`](lgamma.md): the individual gamma factors overflow quickly, while
their logs stay in range and the product becomes a sum.

The parameter `p` changes the function being evaluated; it is not an axis and
does not refer to the rank of `self`. A vector of candidate scalar arguments
remains a vector after `mvlgamma(3)`. The `3` says that each candidate is being
used in a three-dimensional matrix normalizer.

Anchor facts:

- `mvlgamma(x, 1) = lgamma(x)` exactly — the sum has a single term and the
  `pi` prefactor is `pi^0 = 1`.
- The argument must satisfy `x > (p - 1)/2`, so that every gamma factor
  `x + (1 - j)/2` has a positive argument. Outside that half-line `mvlgamma()`
  returns `NaN`.

`mvlgamma()` is out-of-place; `mvlgamma_()` overwrites the current storage.

## Parameters

**p** (`std::int64_t`): the matrix dimension. Must be `>= 1`.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `mvlgamma(x, p)`, or `NaN` where `x <= (p - 1)/2`. `mvlgamma_()` returns a
    reference to the mutated series.

An integral input is promoted to `float64`.

## Gradient

`mvlgamma(., p)` is differentiable with respect to `x`. Differentiating the sum
term by term gives a sum of digamma values:

```
d/dx mvlgamma(x, p) = sum_{j=1..p} psi( x + (1 - j)/2 )
```

For `p = 1` that is just `psi(x)`, matching `lgamma`. `p` is a structural
argument and carries no gradient. Where the forward value is `NaN` (outside the
domain) the sensitivity is `NaN` too.

## In-Place Form

`mvlgamma_()` is the in-place sibling of [`mvlgamma()`](mvlgamma.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `mvlgamma_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.mvlgamma_(1);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0.69314718]
```

## Examples

Example 1. `p = 1` reproduces [`lgamma`](lgamma.md).

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({2.0, 3.0, 4.0}).mvlgamma(1)) << "\n";
pluss::series::print(std::cout,
                     pluss::make_series({2.0, 3.0, 4.0}).lgamma()) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.6931, 1.7918]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.6931, 1.7918]
```

Example 2. Hold the candidate arguments fixed and change `p`. This isolates
what the dimension parameter contributes: more shifted gamma terms and a
larger `pi` prefactor, but no change to the shape of the returned series.

```cpp
auto x = pluss::make_series({3.0, 4.0});
pluss::series::print(std::cout, x.mvlgamma(1)) << "\n";
pluss::series::print(std::cout, x.mvlgamma(2)) << "\n";
pluss::series::print(std::cout, x.mvlgamma(3)) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6931, 1.7918]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.5502, 3.5651]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.6949, 5.4030]
```

Example 3. The domain is `x > (p - 1)/2`. For `p = 2` that means `x > 0.5`;
`0.4` and `0.5` fall outside and come back `NaN`.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({0.5, 0.6, 1.0}).mvlgamma(2)) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, 3.2233, 1.1447]
```

Example 4. The backward pass is a sum of digamma terms.

```cpp
auto x = pluss::make_series({3.0, 4.0});
x.tracks_slope_(true);
x.mvlgamma(2).sum().reverse();

pluss::series::print(std::cout, x.sensitivity()) << "\n";

auto y = pluss::make_series({3.0, 4.0});
pluss::series::print(std::cout,
                     y.digamma().add(y.sub(0.5).digamma())) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.6259, 2.3593]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.6259, 2.3593]
```

The sensitivity for `p = 2` is `psi(x) + psi(x - 0.5)`, reproduced on the right.

## Gotchas And Failure Modes

- `p` must be `>= 1`. `p = 0` or a negative dimension raises
  `mvlgamma: p must be >= 1`.
- The domain shrinks as `p` grows: `x` must exceed `(p - 1)/2`. Values on or
  below that bound return `NaN`, they do not raise.
- `mvlgamma(x, p)` is not `p * lgamma(x)`; the gamma arguments are shifted by
  half-integers and there is a `pi` prefactor.
- `p` is not inferred from `self.shape()` and does not select a dimension of
  the series. Pass the matrix dimension represented by your statistical model.
- `p` carries no gradient; only `x` does.
- Out-of-place calls promote integral input to `float64`. The in-place form
  requires floating storage because it cannot write fractional results back
  into an integral buffer.

## Summary

`mvlgamma(x, p)` is `ln Gamma_p(x)`, the log multivariate gamma:

- `(p(p-1)/4) ln(pi) + sum_{j=1..p} lgamma(x + (1 - j)/2)`
- `mvlgamma(x, 1)` equals [`lgamma(x)`](lgamma.md)
- defined for `x > (p - 1)/2`; `NaN` outside
- differentiable in `x` with backward `sum_{j} psi(x + (1 - j)/2)`
- `mvlgamma_()` is the in-place form

## See Also

- [`Series::lgamma`](lgamma.md): the `p = 1` case and the per-term building
  block.
- [`Series::digamma`](digamma.md): the summands of the gradient.
- [`Series::beta`](beta.md): another gamma-ratio normalizing constant.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="multinomial.html">
    <span class="label">Previous</span>
    <span class="title">multinomial</span>
  </a>
  <a class="next-link" href="nan_to_num.html">
    <span class="label">Next</span>
    <span class="title">nan_to_num</span>
  </a>
</div>
