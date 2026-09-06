<!-- pluss-hand-authored -->

# Series::erfinv

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::erfinv() const
Series& Series::erfinv_()
```

## What It Does

Computes the inverse error function elementwise. `erfinv` is the functional
inverse of [`erf`](erf.md) on the open interval `(-1, 1)`:

```
erf( erfinv(x) ) = x        for x in (-1, 1)
```

Its main use is turning a uniform or probability-scale quantity into a Gaussian
one. The standard-normal quantile function is a rescaled `erfinv`:

```
Phi^{-1}(p) = sqrt(2) * erfinv(2p - 1)
```

so drawing `p` uniformly and passing it through this relation produces
standard-normal samples, and mapping model probabilities back to a latent
Gaussian scale (a probit link) is the same operation.

Domain and endpoints:

- `erfinv(0) = 0`.
- `erfinv` is odd: `erfinv(-x) = -erfinv(x)`.
- As `x -> 1` the value goes to `+inf`; as `x -> -1` it goes to `-inf`.
  `erfinv(1)` returns `+inf` and `erfinv(-1)` returns `-inf` exactly.
- Outside `[-1, 1]` the inverse does not exist. `erfinv(x)` returns `NaN`
  there — it does **not** clamp the argument back into range.

`erfinv()` is out-of-place; `erfinv_()` overwrites the current storage. The
forward value is computed from a rational approximation refined by two Newton
steps against [`erf`](erf.md), accurate to near machine precision on the
interior.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`. Interior arguments map to finite
    values, `+/-1` to `+/-inf`, and `|x| > 1` to `NaN`. `erfinv_()` returns a
    reference to the mutated series.

An integral input is promoted to `float64`.

## Gradient

`erfinv()` is differentiable on the interior. By the inverse-function rule,
with `y = erfinv(x)` and `erf'(y) = (2/sqrt(pi)) e^(-y^2)`:

```
d/dx erfinv(x) = sqrt(pi)/2 * exp( erfinv(x)^2 )
```

The derivative is `sqrt(pi)/2 = 0.8862` at `x = 0` and grows without bound
toward the endpoints. At `x = +/-1` the forward value is infinite and the
sensitivity is `+inf`; for `|x| > 1` both are `NaN`.

## Examples

Example 1. Interior values, including the odd symmetry.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({0.0, 0.5, -0.5, 0.9}).erfinv());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.4769, -0.4769, 1.1631]
```

Example 2. It inverts [`erf`](erf.md): `erf(erfinv(x))` returns `x`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-0.8, -0.2, 0.3, 0.95}).erfinv().erf());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.8000, -0.2000, 0.3000, 0.9500]
```

Example 3. Endpoints and out-of-domain arguments. `+/-1` give `+/-inf`; any
`|x| > 1` gives `NaN`. The argument is never silently clamped.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, -1.0, 1.5, 2.0, -2.0}).erfinv()
) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[inf, -inf, nan, nan, nan]
```

Example 4. The backward pass is `sqrt(pi)/2 * exp(erfinv(x)^2)` — smallest at
the centre, rising toward the endpoints.

```cpp
auto x = pluss::make_series({0.0, 0.3, 0.6});
x.tracks_slope_(true);
x.erfinv().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.8862, 0.9545, 1.2629]
```

Example 5. Integral inputs are promoted to `float64`. Note that `1` is an
endpoint, so the promoted result is `+inf`.

```cpp
pluss::series::print(std::cout, pluss::make_series({0, 1}).erfinv());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.0000, inf]
```

Example 6. `erfinv_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({0.0, 0.5, 1.0});
a.erfinv_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.4769, inf]
```

Example 7. Convert central probabilities to standard-normal quantiles. The
affine step maps `[0, 1]` to the error-function domain `[-1, 1]`; the final
`sqrt(2)` changes from the error-function scale to standard deviations.

```cpp
auto probabilities = pluss::make_series({0.025, 0.5, 0.975});
auto quantiles = probabilities.mul(2.0)
                              .sub(1.0)
                              .erfinv()
                              .mul(std::sqrt(2.0));
pluss::series::print(std::cout, quantiles) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.9600, 0.0000, 1.9600]
```

## Gotchas And Failure Modes

- Arguments outside `[-1, 1]` return `NaN`; the method never silently clamps
  invalid input. If a probability-scale quantity can acquire small rounding
  excursions, validate or clamp it deliberately before calling.
- `erfinv(+/-1)` is `+/-inf`, not a finite sentinel.
- The gradient blows up near the endpoints; a tracked `erfinv()` on data close
  to `+/-1` produces very large sensitivities.
- For the standard-normal quantile use `sqrt(2) * erfinv(2p - 1)`; `erfinv`
  itself is on the `erf` scale, not the `Phi` scale.
- Integral inputs promote to `float64`.
- `erfinv_()` requires floating storage. Use `erfinv()` when starting from an
  integral series so the result has somewhere to store fractional values.

## Summary

`erfinv()` inverts [`erf`](erf.md) on `(-1, 1)`:

- `erfinv(0) = 0`, odd, `erfinv(+/-1) = +/-inf`
- `|x| > 1` returns `NaN` — no clamping
- differentiable on the interior with backward
  `sqrt(pi)/2 * exp(erfinv(x)^2)`
- the normal quantile is `sqrt(2) * erfinv(2p - 1)`
- `erfinv_()` is the in-place form

## See Also

- [`Series::erf`](erf.md): the function this inverts.
- [`Series::erfcinv`](erfcinv.md): the inverse of the complementary error
  function, better conditioned in the far tail.
- [`Series::erfc`](erfc.md): the complementary error function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="erfcinv.html">
    <span class="label">Previous</span>
    <span class="title">erfcinv</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
