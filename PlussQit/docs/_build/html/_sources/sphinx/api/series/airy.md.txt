<!-- pluss-hand-authored -->

# Series::airy

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::airy() const
```

## The Airy Ai Transform

`airy()` evaluates the Airy function Ai elementwise. Ai is the particular
solution of `y''(x) - x y(x) = 0` that decays toward zero on the positive real
axis. This combination of an oscillating negative half-line and a rapidly
decaying positive half-line makes Ai useful around turning points, where a
differential equation changes from oscillatory to exponential behavior.

The method computes **Ai**, not its independent companion Bi. Three facts help
identify the result: `Ai(0) = 0.3550280538...`; Ai oscillates for negative
arguments; and it remains positive while decaying exponentially for positive
arguments.

This is an out-of-place, elementwise transform. It preserves rank, shape, and
platform. Floating input is evaluated at its floating precision; integral and
boolean input is promoted to `float64`, because Ai is not integer-valued.
Complex input is not part of the real-valued contract.

The central interval is evaluated from the convergent solution of the Airy
differential equation. The tails use asymptotic forms so large arguments do
not force badly conditioned power-series cancellation. Positive arguments
beyond the representable exponential tail approach exact `0.0`. Infinite real
input returns `NaN`; no finite oscillatory value exists at infinity.

## Reverse Mode

`airy()` records a unary reverse node. If `a = x.airy()`, then
`da/dx = Ai'(x)`, so an incoming sensitivity is multiplied elementwise by
`Ai'(x)`. Shape is unchanged and no broadcast reduction is required. The
derivative at zero is `Ai'(0) = -0.2588194038...`.

## Examples

Example 1. Values on both sides of the turning point show the oscillatory and
decaying regimes.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).airy());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.2274, 0.5356, 0.3550, 0.1353, 0.0349]
```

Example 2. The transform is lane-wise. A matrix remains a matrix, and integer
storage is promoted before evaluation.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{-1, 0}, {1, 2}}).airy());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.5356 0.3550 ⎤
⎣ 0.1353 0.0349 ⎦
```

Example 3. Summing seeds every output lane with one, so the stored sensitivity
is `Ai'` at each input lane.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.airy().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.0102, -0.2588, -0.1591]
```

Example 4. Scalar rank is preserved; a scalar does not become a one-element
vector.

```cpp
const auto a = pluss::make_series(0.0).airy();
std::cout << "dim=" << a.dim() << " value=" << a.item<double>() << "\n";
```

### Output

```text
dim=0 value=0.355028
```

## See Also

- [`besselj()`](besselj.md) is another oscillatory special function.
- [`erf()`](erf.md) is an elementwise integral-defined special function.
- [`reverse()`](reverse.md) seeds and propagates sensitivities.
