<!-- pluss-hand-authored -->

# Series::beta

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::beta(double other) const
```

## What It Does

`beta()` computes the beta function `B(a, b)` elementwise. `self` supplies
`a`; the argument supplies `b`:

```
B(a, b) = Gamma(a) * Gamma(b) / Gamma(a + b)
```

The beta function is the normalizing constant of the Beta distribution and the
value of the Euler integral `integral_0^1 t^(a-1) (1 - t)^(b-1) dt`. It appears
in the Beta and Dirichlet densities, in the binomial coefficient
(`1 / ((n + 1) B(n - k + 1, k + 1))`), and in the `t` and `F` distribution
normalizers.

Use [`betaln()`](betaln.md) when the normalizer belongs in a logarithmic
calculation; `beta()` can underflow even while the logarithm remains finite.

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
    `beta()` returns `B(a, b)` with the same shape as `self`. An integral input
    is promoted to `float64`.

## Gradient

`beta()` is differentiable with respect to `a` (`self`). Its derivative is:

```
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

Example 2. The beta function is symmetric in its arguments.

```cpp
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
```

Example 3. The backward pass carries the `B(a, b)` factor.

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

## Summary

`beta(b)` is `B(a, b) = Gamma(a) Gamma(b) / Gamma(a + b)`, with `a` from
`self`:

- symmetric; `B(a, 1) = 1/a`
- prefer [`betaln()`](betaln.md) for log-domain calculations because `beta()`
  loses range
- differentiable in `a`; the backward multiplies the digamma difference by
  `B(a, b)`
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
  <a class="next-link" href="betaln.html">
    <span class="label">Next</span>
    <span class="title">betaln</span>
  </a>
</div>
