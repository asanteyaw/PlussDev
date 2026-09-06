<!-- pluss-hand-authored -->

# Series::igamma

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::igamma(const Series& other) const
Series Series::igamma(double other) const
Series& Series::igamma_(const Series& other)
Series& Series::igamma_(double other)
```

## What It Does

Computes the regularized lower incomplete gamma function `P(a, x)` elementwise.
`self` supplies the shape parameter `a`; the argument supplies the upper limit
`x`:

```
P(a, x) = ( 1 / Gamma(a) ) * integral_0^x t^(a-1) e^(-t) dt
```

The regularization by `Gamma(a)` puts the value on `[0, 1]`, and `P(a, .)` is
exactly the cumulative distribution function of a Gamma(`a`, rate `1`) random
variable. So `igamma` is how you get Gamma, chi-square, and Erlang tail
probabilities: the chi-square CDF with `k` degrees of freedom is
`P(k/2, x/2)`.

Behaviour:

- `P(a, 0) = 0` and `P(a, x) -> 1` as `x -> inf`; the function is monotone
  increasing in `x`.
- `P(a, x) + Q(a, x) = 1`, where `Q` is [`igammac`](igammac.md), the upper
  tail.
- Defined for `a > 0` and `x >= 0`. Outside that the result is `NaN`.

The `double` overload treats `x` as a constant broadcast against every element
of `a`.

The Series overload follows ordinary broadcasting. This matters when a column
of shape parameters is evaluated at a row of integration limits: the result is
a table whose rows are distributions and whose columns are evaluation points.

## Parameters

**other** (`const Series&` or `double`): the upper limit `x`, broadcast against
`self`.

## Returns

`Series` or `Series&`
    A `Series` broadcast over `a` and `x`, each element `P(a, x)` in `[0, 1]`.
    The in-place forms write the broadcast result into `self` and therefore
    require the broadcast shape to equal `self.shape()`.

Integral inputs are promoted for out-of-place evaluation. In-place evaluation
requires floating storage.

## Gradient

`igamma` is differentiable **with respect to `x`** (the `other` argument, when
it is a `Series`). Differentiating the integral by its upper limit:

```
d/dx P(a, x) = x^(a-1) e^(-x) / Gamma(a)
```

which is the Gamma(`a`, `1`) density at `x`.

There is **no gradient with respect to `a`** (the shape parameter): that
derivative has no elementary closed form and is not wired. If `self` tracks
slope and `x` does not, an objective built only from `igamma` is disconnected
from its input, and calling [`reverse()`](reverse.md) on it raises. The
`igamma(double)` overload holds `x` constant, so it is non-differentiable in
the same way.

## Examples

Example 1. With `a = 2` fixed, `P` climbs from `0` toward `1` as `x` grows.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0, 2.0, 2.0, 2.0})
        .igamma(pluss::make_series({0.0, 1.0, 3.0, 20.0}))
) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.2642, 0.8009, 1.0000]
```

Example 2. The lower and upper tails sum to `1`.

```cpp
auto a = pluss::make_series({0.5, 1.0, 2.5});
auto x = pluss::make_series({0.3, 1.0, 4.0});
pluss::series::print(std::cout, a.igamma(x).add(a.igammac(x)));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 1.0000, 1.0000]
```

Example 3. At `a = 1` the incomplete gamma collapses to the exponential CDF,
`P(1, x) = 1 - e^(-x)`.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
pluss::series::print(std::cout, pluss::make_series({1.0, 1.0, 1.0}).igamma(x));
pluss::series::print(std::cout, x.neg().exp().neg().add(1.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.3935, 0.6321, 0.8647]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.3935, 0.6321, 0.8647]
```

Example 4. The backward pass with respect to `x` is the Gamma density
`x^(a-1) e^(-x) / Gamma(a)`.

```cpp
auto x = pluss::make_series({1.0, 3.0});
x.tracks_slope_(true);
pluss::make_series({2.0, 2.0}).igamma(x).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.3679, 0.1494]
```

With `a = 2`, the density is `x e^(-x)`: `1 * e^(-1) = 0.3679` and
`3 * e^(-3) = 0.1494`.

Example 5. Broadcasting a `[2,1]` column of shapes against three limits builds
a `[2,3]` CDF table. The first row has `a = 1`; the second has `a = 2`.

```cpp
auto shapes = pluss::make_series({1.0, 2.0}).reshape({2, 1});
auto limits = pluss::make_series({0.5, 1.0, 2.0});
pluss::series::print(std::cout, shapes.igamma(limits)) << "\n";
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0.3935 0.6321 0.8647 ⎤
⎣ 0.0902 0.2642 0.5940 ⎦
```

Example 6. Exact endpoints pair naturally with the complementary function.
At `x = 0`, all mass is in the upper tail; at `x = +inf`, all mass is in the
lower tail.

```cpp
auto a = pluss::make_series({2.0, 2.0, 2.0});
auto x = pluss::make_series({0.0, 3.0,
                             std::numeric_limits<double>::infinity()});
pluss::series::print(std::cout, a.igamma(x)) << "\n";
pluss::series::print(std::cout, a.igammac(x)) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.8009, 1.0000]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 0.1991, 0.0000]
```

## Gotchas And Failure Modes

- Argument order is `a.igamma(x)`: `self` is the shape parameter, the argument
  is the integration limit. Swapping them silently computes a different value.
- Gradient flows only through `x`, and only when `x` is a `Series`. There is no
  gradient with respect to `a`; an objective that depends on `a` alone through
  `igamma` is disconnected and `reverse()` raises.
- `a <= 0` or `x < 0` gives `NaN`.
- For the upper tail (survival function) use [`igammac`](igammac.md) rather
  than `1 - igamma`, which loses precision when `P` is close to `1`.
- In-place broadcasting cannot enlarge the receiver. If broadcasting `a` and
  `x` would create a larger shape, use `igamma()` and keep the returned series.

## Summary

`igamma(x)` is the regularized lower incomplete gamma `P(a, x)`, with `a` from
`self`:

- the CDF of a Gamma(`a`, `1`); chi-square CDF is `P(k/2, x/2)`
- rises from `0` to `1` in `x`; `P + Q = 1` with [`igammac`](igammac.md)
- `NaN` for `a <= 0` or `x < 0`
- differentiable in `x` with backward `x^(a-1) e^(-x) / Gamma(a)`; **not**
  differentiable in `a`
- the `double` overload holds `x` constant and is non-differentiable

## See Also

- [`Series::igammac`](igammac.md): the complementary upper tail `Q(a, x)`.
- [`Series::lgamma`](lgamma.md): the `Gamma(a)` normalizer, in log space.
- [`Series::erf`](erf.md): `P(1/2, x^2)` is `erf(x)`, a special case.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="i0.html">
    <span class="label">Previous</span>
    <span class="title">i0</span>
  </a>
  <a class="next-link" href="igammac.html">
    <span class="label">Next</span>
    <span class="title">igammac</span>
  </a>
</div>
