<!-- pluss-hand-authored -->

# Series::erf

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::erf() const -> Series
Series& Series::erf_()
```

## What It Does

Applies the Gauss error function elementwise.

For a scalar input `x`, the error function is the area under a bell curve,
scaled so that it approaches `1`:

```
erf(x) = (2 / sqrt(pi)) * integral from 0 to x of exp(-t^2) dt
```

That integral has no elementary closed form, so `erf()` is a primitive: the
library evaluates it directly rather than composing it from `exp`, `log`, and
friends.

A few properties fall straight out of the definition and are worth carrying in
your head:

- `erf(0) = 0`, because the integral over an empty interval is zero.
- `erf(-x) = -erf(x)`. The integrand `exp(-t^2)` is symmetric about `t = 0`, so
  the signed area is an odd function.
- `erf(x)` is bounded. As `x -> +inf` it approaches `+1`; as `x -> -inf` it
  approaches `-1`. The `2 / sqrt(pi)` factor is exactly what makes those limits
  land on `+/-1` rather than on `+/-sqrt(pi)/2`.
- `erf` is strictly increasing, because its integrand is strictly positive
  everywhere.

The single most common reason to reach for `erf()` is its link to the normal
distribution. The cumulative distribution function of a standard normal is

```
Phi(x) = 0.5 * (1 + erf(x / sqrt(2)))
```

so anything phrased in terms of "probability a Gaussian draw falls below a
threshold" is an `erf()` call in disguise. The Gaussian-error-linear-unit
activation is built the same way.

`erf()` is an out-of-place unary transform: shape, dtype, and platform are
preserved, no broadcasting is involved, and the result is a fresh `Series`.
`erf_()` is the in-place spelling that overwrites the current storage.

## Parameters

None.

The transform is fully determined by the input values.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `erf(x)`. The out-of-place overload returns a new value; `erf_()` returns a
    reference to the mutated current series.

The result dtype matches the input dtype for the supported floating-point
types. A `float32` input produces a `float32` result; a `float64` input
produces a `float64` result.

## Gradient

`erf()` is differentiable, and its backward rule is the derivative of the
defining integral. By the fundamental theorem of calculus, differentiating the
integral just evaluates the integrand at the upper limit:

```
d/dx erf(x) = (2 / sqrt(pi)) * exp(-x^2)
```

The constant `2 / sqrt(pi)` is `1.1283791670955126`. So the local multiplier is
largest at `x = 0`, where it equals that constant exactly, and decays like a
Gaussian as `|x|` grows. Far out in either tail the sensitivity is effectively
zero, which mirrors the fact that `erf()` has flattened against `+/-1` and is
no longer responding to its input.

Because the backward multiplier is itself `exp(-x^2)` times a constant, it is
smooth and composes cleanly through the chain rule with any surrounding
computation.

## Examples

The examples build up from the basic shape to dtype handling, the autograd
rule, and the failure mode.

Example 1. The signed, bounded, odd shape on a symmetric grid of inputs.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0}).erf());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[-0.9953, -0.8427, -0.5205, 0.0000, 0.5205, 0.8427, 0.9953]
```

The output is antisymmetric around the middle entry, and the middle entry is
exactly `0`.

Example 2. Saturation. Once `|x|` is past roughly `4`, the result is `+/-1` to
the precision shown.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-4.0, -3.0, 3.0, 4.0, 10.0}).erf());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.0000, -1.0000, 1.0000, 1.0000, 1.0000]
```

This is not an overflow. `erf` is bounded, so large-magnitude inputs simply map
to the horizontal asymptotes.

Example 3. The transform is elementwise and shape-preserving on matrices.

```cpp
std::cout << pluss::make_series({
    {0.0, 0.5},
    {1.0, 2.0},
}).erf();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0.52049988],
  [0.84270079, 0.99532227]]
```

Example 4. A non-contiguous input is accepted. The result is a fresh
contiguous series carrying the transformed values in the view's visible order.

```cpp
auto rows = pluss::make_series({
    {-1.0, 0.0, 1.0},
    {2.0, 3.0, 4.0},
});

std::cout << rows.transpose(0, 1).erf();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-0.84270079, 0.99532227],
  [0, 0.99997791],
  [0.84270079, 0.99999998]]
```

Example 5. `float32` in, `float32` out. `erf()` does not silently widen to
`float64`.

```cpp
auto f = pluss::make_series({-1.0f, 0.0f, 1.0f}, pluss::ScalarType::Float32);

pluss::series::print(std::cout, f.erf());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[-0.8427, 0.0000, 0.8427]
```

Example 6. Non-finite inputs follow the limits. `NaN` propagates, and the
infinities map to the asymptotes.

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
double inf = std::numeric_limits<double>::infinity();

pluss::series::print(std::cout, pluss::make_series({nan, inf, -inf}).erf());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, 1.0000, -1.0000]
```

Example 7. The backward multiplier is `(2 / sqrt(pi)) * exp(-x^2)`. Here the
reverse-mode sensitivity is compared against that formula evaluated by hand.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
x.tracks_slope_(true);

auto y = x.erf();
y.sum().reverse();

std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";

auto manual = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
std::cout << "by formula=" << manual.mul(manual).neg().exp().mul(1.1283791670955126);
```

### Output

```text
y=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.84270079, 0, 0.52049988, 0.84270079]
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.4151075, 1.1283792, 0.87878258, 0.4151075]
by formula=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.4151075, 1.1283792, 0.87878258, 0.4151075]
```

The sensitivity peaks at `x = 0` with value `1.1283792`, which is
`2 / sqrt(pi)`, and it is already symmetric because the derivative depends only
on `x^2`.

Example 8. The rule composes. For `erf(2x)` the chain rule multiplies the local
derivative by the inner slope `2`:

```
d/dx erf(2x) = 2 * (2 / sqrt(pi)) * exp(-(2x)^2)
```

```cpp
auto x = pluss::make_series({0.0, 0.5, 1.0});
x.tracks_slope_(true);

auto y = x.mul(2.0).erf();
y.sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.2568, 0.8302, 0.0413]
```

At `x = 0` the value is `2 * 1.1283792 = 2.2568`. By `x = 1` the inner argument
is `2`, `exp(-4)` is tiny, and the sensitivity has nearly vanished.

Example 9. `erf()` is floating-point only. An integer or boolean input does not
promote; it raises.

```cpp
try {
  pluss::series::print(std::cout, pluss::make_series({0, 1, 2}).erf());
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
erf: dtype not implemented (float-only)
```

Convert first when the source column is integral:

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0, 1, 2}).to(pluss::ScalarType::Float64).erf());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.8427, 0.9953]
```

Example 10. `erf_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({0.0, 1.0, 2.0});
a.erf_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.8427, 0.9953]
```

## Gotchas And Failure Modes

- `erf()` requires a floating-point input. Integer and boolean series raise
  `erf: dtype not implemented (float-only)`; call [`to()`](to.md) first.
- The result is bounded in `(-1, 1)`. Large-magnitude inputs are not an error
  and do not overflow; they saturate against `+/-1`.
- Because the backward multiplier decays like `exp(-x^2)`, gradients through
  `erf()` are effectively zero once `|x|` is large. A saturated `erf()` stops
  passing useful sensitivity to its input.
- `erf()` is not `erfc()`. For tail probabilities where `erf(x)` is within
  rounding distance of `1`, compute `erfc(x)` directly instead of `1 - erf(x)`.
- `NaN` propagates unchanged.

## Summary

`erf()` is the Gauss error function as a shape-preserving unary transform:

- odd, strictly increasing, bounded in `(-1, 1)`
- the bridge between raw inputs and standard-normal probabilities
- differentiable with backward multiplier `(2 / sqrt(pi)) * exp(-x^2)`
- floating-point only; convert integral inputs with [`to()`](to.md)
- `erf_()` is the in-place form

## See Also

- [`Series::erfc`](erfc.md): the complementary error function `1 - erf(x)`,
  accurate in the tail where `erf(x)` rounds to `1`.
- [`Series::exp`](exp.md): the primitive that the backward rule of `erf()` is
  built from.
- [`Series::sigmoid`](sigmoid.md): another bounded, saturating, differentiable
  activation-style transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equal.html">
    <span class="label">Previous</span>
    <span class="title">equal</span>
  </a>
  <a class="next-link" href="erfc.html">
    <span class="label">Next</span>
    <span class="title">erfc</span>
  </a>
</div>
