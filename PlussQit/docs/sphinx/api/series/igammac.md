<!-- pluss-hand-authored -->

# Series::igammac

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::igammac(const Series& other) const
Series Series::igammac(double other) const
Series& Series::igammac_(const Series& other) const
```

## What It Does

Computes the regularized *upper* incomplete gamma function `Q(a, x)`
elementwise — the complement of [`igamma`](igamma.md). `self` is the shape
parameter `a`, the argument is the lower limit `x`:

```
Q(a, x) = ( 1 / Gamma(a) ) * integral_x^inf t^(a-1) e^(-t) dt = 1 - P(a, x)
```

`Q(a, .)` is the survival function of a Gamma(`a`, rate `1`) variable, so
`igammac` gives right-tail probabilities directly: the chi-square survival
function with `k` degrees of freedom is `Q(k/2, x/2)`, which is what a
goodness-of-fit p-value needs.

The reason to call `igammac` rather than `1 - igamma(x)`: when the upper tail
is small, `P(a, x)` is close to `1` and `1 - P` cancels most of its
significant digits. `Q` is computed on its own footing and stays accurate deep
into the tail — note the `4.3e-08` in Example 1, a value `1 - igamma` could not
resolve.

Behaviour:

- `Q(a, 0) = 1` and `Q(a, x) -> 0` as `x -> inf`; monotone decreasing in `x`.
- `P(a, x) + Q(a, x) = 1`.
- Defined for `a > 0` and `x >= 0`; `NaN` outside.

## Parameters

**other** (`const Series&` or `double`): the lower limit `x`, broadcast against
`self`.

## Returns

`Series`
    A `Series` broadcast over `a` and `x`, each element `Q(a, x)` in `[0, 1]`.

## Gradient

`igammac` is differentiable **with respect to `x`** (the `other` argument, when
it is a `Series`). It is the negative of the [`igamma`](igamma.md) derivative,
since `Q = 1 - P`:

```
d/dx Q(a, x) = -x^(a-1) e^(-x) / Gamma(a)
```

There is **no gradient with respect to `a`**. As with `igamma`, an objective
that reaches its input only through the shape parameter — or through the
`igammac(double)` overload, which holds `x` constant — is disconnected, and
[`reverse()`](reverse.md) on it raises.

## In-Place Form

`igammac_()` is the in-place sibling of [`igammac()`](igammac.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `igammac_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0});
auto y = pluss::make_series({1.0, 1.0});

x.igammac_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.36787944, 0.73575888]
```

## Examples

Example 1. With `a = 2` fixed, `Q` falls from `1` toward `0` as `x` grows, and
stays accurate in the far tail.

```cpp
std::cout << pluss::make_series({2.0, 2.0, 2.0, 2.0})
                 .igammac(pluss::make_series({0.0, 1.0, 3.0, 20.0}));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.73575888, 0.19914827, 4.3284226e-08]
```

Example 2. At `a = 1` the upper tail is the exponential survival function,
`Q(1, x) = e^(-x)`.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
pluss::series::print(std::cout, pluss::make_series({1.0, 1.0, 1.0}).igammac(x));
pluss::series::print(std::cout, x.neg().exp());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6065, 0.3679, 0.1353]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6065, 0.3679, 0.1353]
```

Example 3. The backward pass with respect to `x` is the Gamma density negated.

```cpp
auto x = pluss::make_series({1.0, 3.0});
x.tracks_slope_(true);
pluss::make_series({2.0, 2.0}).igammac(x).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.3679, -0.1494]
```

Same magnitudes as the [`igamma`](igamma.md) example, opposite sign.

## Gotchas And Failure Modes

- Argument order is `a.igammac(x)`: `self` is the shape parameter.
- Prefer `igammac(x)` to `1 - igamma(x)` for small tails — the subtraction
  loses precision exactly where you need it.
- Gradient flows only through `x`, and only when `x` is a `Series`. No gradient
  with respect to `a`; a shape-only objective is disconnected and `reverse()`
  raises.
- `a <= 0` or `x < 0` gives `NaN`.

## Summary

`igammac(x)` is the regularized upper incomplete gamma `Q(a, x) = 1 - P(a, x)`,
with `a` from `self`:

- the survival function of a Gamma(`a`, `1`); chi-square survival is
  `Q(k/2, x/2)`
- falls from `1` to `0` in `x`; accurate in the far tail where `1 - igamma`
  is not
- `NaN` for `a <= 0` or `x < 0`
- differentiable in `x` with backward `-x^(a-1) e^(-x) / Gamma(a)`; **not** in
  `a`

## See Also

- [`Series::igamma`](igamma.md): the lower tail `P(a, x)`.
- [`Series::lgamma`](lgamma.md): the `Gamma(a)` normalizer.
- [`Series::erfc`](erfc.md): the complementary error function, the analogous
  "compute the small tail directly" routine for the Gaussian.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="igamma.html">
    <span class="label">Previous</span>
    <span class="title">igamma</span>
  </a>
  <a class="next-link" href="imag.html">
    <span class="label">Next</span>
    <span class="title">imag</span>
  </a>
</div>
