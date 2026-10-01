<!-- pluss-hand-authored -->

# Series::selu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::selu() const
Series& Series::selu_() const
```

## Fixed Scaled Exponential Activation

`selu()` applies a scaled exponential linear unit with fixed constants:

$$
\operatorname{selu}(x)=\lambda
\begin{cases}
x, & x>0,\\
\alpha(e^x-1), & x\le 0,
\end{cases}
$$

where `alpha` is approximately `1.6732632423543772` and `lambda` is
approximately `1.0507009873554805`. The fixed pair is the operation's contract;
use [`elu()`](elu.md) when those coefficients need to be controlled directly.

The positive branch is linear with slope greater than one. The negative branch
saturates toward `-lambda * alpha`, approximately `-1.7581`, instead of
decreasing without bound. This asymmetric shape is intended for networks whose
initialization and architecture are designed around self-normalizing behavior;
the activation alone does not guarantee that property for an arbitrary model.

## Contract

- Evaluation is elementwise and preserves shape, rank, and platform.
- Floating dtype is preserved; integer and Boolean input is promoted to
  `float32`.
- Complex input is rejected.
- The method is out of place and leaves the receiver unchanged.
- Reverse mode uses slope `lambda` on positive lanes and
  `lambda * alpha * exp(x)` on non-positive lanes. At exactly zero, the
  non-positive branch is used, giving slope `lambda * alpha`.

## In-Place Form

`selu_()` is the in-place sibling of [`selu()`](selu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `selu_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.selu_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.5201665, 0, 2.101402]
```

## Examples

### 1. Compare the exponential and linear regions

```cpp
const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
pluss::series::print(std::cout, x.selu());
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.6706, -1.1113, 0.0000, 1.0507, 3.1521]
```

The negative values bend toward the finite lower asymptote. Positive values are
scaled by the fixed factor `lambda`.

### 2. Shape is preserved for matrices

```cpp
const auto x = pluss::make_series({{-2.0, 0.0}, {1.0, 3.0}});
pluss::series::print(std::cout, x.selu());
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -1.5202 0.0000 ⎤
⎣  1.0507 3.1521 ⎦
```

### 3. Integer lanes are promoted before activation

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({-2, 0, 2}).selu());
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[-1.5202, 0.0000, 2.1014]
```

The result is `float32`; neither the exponential branch nor the fixed scale is
truncated to an integer.

### 4. Reverse mode distinguishes the zero boundary

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.selu().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6468, 1.7581, 1.0507]
```

At zero the derivative comes from the exponential branch and is approximately
`1.7581`; immediately on the positive side it is approximately `1.0507`.

### 5. Complex input is outside the contract

```cpp
try {
  using Complex = std::complex<double>;
  (void)pluss::make_series(
      std::vector<Complex>{{1.0, 1.0}}, {1}).selu();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
selu: complex input is not supported
```

## Performance

The operation is linear in `numel()`. Positive lanes require only scaling;
negative lanes require an exponential. The analytic activation node keeps the
forward tape compact and supports reverse and higher-order differentiation.

## See Also

- [`elu()`](elu.md) exposes the related coefficients.
- [`mish()`](mish.md) is smooth across zero and does not saturate to the same
  negative constant.
- [`leaky_relu()`](leaky_relu.md) uses a linear negative branch.
