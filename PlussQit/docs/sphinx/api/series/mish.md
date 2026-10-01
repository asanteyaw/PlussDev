<!-- pluss-hand-authored -->

# Series::mish

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::mish() const
Series& Series::mish_() const
```

## A Smooth, Non-Monotonic Activation

`mish()` applies

$$
\operatorname{mish}(x)=x\tanh(\log(1+e^x))
$$

elementwise. It keeps large positive values close to the identity, softly
suppresses negative values, and permits a small negative output instead of
clamping the entire negative half-line to zero. The shallow negative trough is
why the function is non-monotonic.

Choose it when a smooth activation with nonzero negative output is useful.
Choose [`hardswish()`](hardswish.md) when a cheaper piecewise-polynomial gate is
more important, or [`selu()`](selu.md) when the fixed self-normalizing scale and
negative saturation are specifically required.

## Type, Shape, And Derivative Contract

- The operation is elementwise and preserves rank, shape, and platform.
- Floating-point dtype is preserved. Integer and Boolean receivers are promoted
  to `float32`, because fractional activation values cannot be represented in
  their original dtype.
- Complex input is rejected; the ordered real activation has no unambiguous
  complex extension in this API.
- The receiver is not mutated. The returned Series owns a new result.
- Reverse mode uses the analytic derivative

  $$
  \tanh(s)+x\,\operatorname{sigmoid}(x)(1-\tanh^2(s)),
  \qquad s=\operatorname{softplus}(x).
  $$

  The derivative remains connected for higher-order differentiation.

At very negative input, the result approaches zero from below. At very
positive input, it approaches `x`. The implementation uses the stable
`softplus()` path rather than directly evaluating an overflowing `exp(x)`.

## In-Place Form

`mish_()` is the in-place sibling of [`mish()`](mish.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `mish_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.mish_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.25250148, 0, 1.943959]
```

## Examples

### 1. Observe both tails and the negative trough

```cpp
const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
pluss::series::print(std::cout, x.mish());
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.1456, -0.3034, 0.0000, 0.8651, 2.9865]
```

The value at `-1` is more negative than at `-3`, demonstrating the
non-monotonic negative region. Positive values increasingly approach the
identity.

### 2. Higher-rank input keeps its structure

```cpp
const auto x = pluss::make_series({{-2.0, 0.0}, {1.0, 3.0}});
pluss::series::print(std::cout, x.mish());
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -0.2525 0.0000 ⎤
⎣  0.8651 2.9865 ⎦
```

No axis has special meaning. Every lane is transformed independently.

### 3. Integer input is promoted

```cpp
const auto y = pluss::make_series({-2, 0, 2}).mish();
pluss::series::print(std::cout, y);
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[-0.2525, 0.0000, 1.9440]
```

Promotion happens before evaluation, so the negative and positive fractional
results are not truncated back to integers.

### 4. Reverse mode follows the smooth derivative

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.mish().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0592, 0.6000, 1.0490]
```

The derivative at zero is exactly `0.6`. Near `1`, the derivative can exceed
one; the activation is not a clipped gate.

### 5. Complex input fails explicitly

```cpp
try {
  using Complex = std::complex<double>;
  (void)pluss::make_series(
      std::vector<Complex>{{1.0, 1.0}}, {1}).mish();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
mish: complex input is not supported
```

## Performance

`mish()` is linear in `numel()` and uses exponential, logarithmic, and
hyperbolic operations, so it is materially more expensive than piecewise
linear activations. The named reverse node avoids retaining every temporary
from the forward formula, while recomputing the derivative ingredients only
when reverse propagation is requested.

## See Also

- [`softplus()`](softplus.md) is the smooth positive transform inside Mish.
- [`hardswish()`](hardswish.md) is a cheaper piecewise activation.
- [`selu()`](selu.md) supplies fixed self-normalizing constants.
