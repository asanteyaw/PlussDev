<!-- pluss-hand-authored -->

# Series::beta

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::beta(double other) const
Series Series::betaln(double other) const
```

## What It Does

`beta()` computes the beta function `B(a, b)` elementwise, and `betaln()`
computes its natural log. `self` supplies `a`; the argument supplies `b`:

```
B(a, b) = Gamma(a) * Gamma(b) / Gamma(a + b)
betaln(a, b) = lgamma(a) + lgamma(b) - lgamma(a + b)
```

The beta function is the normalizing constant of the Beta distribution and the
value of the Euler integral `integral_0^1 t^(a-1) (1 - t)^(b-1) dt`. It appears
in the Beta and Dirichlet densities, in the binomial coefficient
(`1 / ((n + 1) B(n - k + 1, k + 1))`), and in the `t` and `F` distribution
normalizers.

`betaln()` is the form to prefer when the result will be multiplied into a
likelihood or other product. The implementation combines three `lgamma`
values, avoiding construction of the enormous intermediate gamma factors.
`beta()` is `exp(betaln())`; it is convenient when the bare normalizer is
needed, but can underflow to zero even while the finite logarithm remains
useful.

Properties:

- Symmetric: `B(a, b) = B(b, a)`.
- `B(1, 1) = 1`; `B(a, 1) = 1/a`.
- `B(a, b) > 0` for `a, b > 0`; poles of the gamma factors at non-positive
  integer arguments propagate through.

## Parameters

**other** (`double`): the second shape parameter `b`, a constant broadcast
against every element of `self`.

## Returns

`Series`
    `beta()` returns `B(a, b)`; `betaln()` returns `ln B(a, b)`. Same shape as
    `self`. An integral input is promoted to `float64`.

## Gradient

Both forms are differentiable with respect to `a` (`self`). Because they are
built from [`lgamma`](lgamma.md), whose derivative is [`digamma`](digamma.md),
the chain rule gives:

```
d/da betaln(a, b) = psi(a) - psi(a + b)
d/da beta(a, b)   = B(a, b) * ( psi(a) - psi(a + b) )
```

`b` is a constant `double` and carries no gradient. Where a gamma factor sits
on a pole the sensitivity is `NaN`.

## Examples

Example 1. `B(2, 3) = 1/12`; `B(3, 3) = 1/30`; `B(1, 3) = 1/3`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 3.0, 1.0}).beta(3.0)
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0833, 0.0333, 0.3333]
```

Example 2. `betaln()` is the log of the same values.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 3.0, 1.0}).betaln(3.0)
) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2.4849, -3.4012, -1.0986]
```

`betaln(2, 3) = ln(1/12) = -2.4849`.

Example 3. `beta()` is `exp(betaln())`, and both are symmetric in their
arguments.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({2.5}).betaln(4.0).exp()) << "\n";
pluss::series::print(std::cout,
                     pluss::make_series({2.5}).beta(4.0)) << "\n";
pluss::series::print(std::cout,
                     pluss::make_series({4.0}).beta(2.5)) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.0277]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.0277]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.0277]
```

Example 4. The backward pass of `betaln()` is `psi(a) - psi(a + b)`.

```cpp
auto a = pluss::make_series({2.0, 4.0});
a.tracks_slope_(true);
a.betaln(3.0).sum().reverse();

pluss::series::print(std::cout, a.sensitivity());

auto aa = pluss::make_series({2.0, 4.0});
pluss::series::print(std::cout, aa.digamma().sub(aa.add(3.0).digamma()));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.0833, -0.6167]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.0833, -0.6167]
```

Example 5. The backward pass of `beta()` carries the extra `B(a, b)` factor.

```cpp
auto a = pluss::make_series({2.0, 4.0});
a.tracks_slope_(true);
a.beta(3.0).sum().reverse();

pluss::series::print(std::cout, a.sensitivity());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.0903, -0.0103]
```

`B(2, 3) * (psi(2) - psi(5)) = (1/12) * (-1.0833) = -0.0903`.

## Gotchas And Failure Modes

- Reach for `betaln()` when accumulating log probabilities. `beta()` can
  underflow for large positive arguments where `betaln()` is still finite.
- `b` is a constant and carries no gradient; only `a` does.
- Non-positive integer values of `a`, `b`, or `a + b` land on gamma poles and
  propagate `inf` / `NaN`.
- `betaln()` is `lgamma(a) + lgamma(b) - lgamma(a + b)` — a difference of logs,
  not the log of a difference.

## Summary

`beta(b)` is `B(a, b) = Gamma(a) Gamma(b) / Gamma(a + b)` and `betaln(b)` its
log, with `a` from `self`:

- symmetric; `B(a, 1) = 1/a`
- prefer `betaln()` — `beta()` is `exp(betaln())` and loses range
- differentiable in `a`: `betaln` backward is `psi(a) - psi(a + b)`, `beta`
  backward multiplies that by `B(a, b)`
- `b` carries no gradient

## See Also

- [`Series::lgamma`](lgamma.md): the building block; `betaln` is a combination
  of three `lgamma` calls.
- [`Series::digamma`](digamma.md): the gradient of `betaln`.
- [`Series::mvlgamma`](mvlgamma.md): the matrix-argument generalization of the
  gamma normalizer.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="baddbmm.html">
    <span class="label">Previous</span>
    <span class="title">baddbmm</span>
  </a>
  <a class="next-link" href="bernoulli.html">
    <span class="label">Next</span>
    <span class="title">bernoulli</span>
  </a>
</div>
