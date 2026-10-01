<!-- pluss-hand-authored -->

# Series::elu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::elu(
    double alpha = 1.0,
    double scale = 1.0,
    double input_scale = 1.0) const
Series& Series::elu_() const
Series& Series::elu_(double alpha) const
```

## The General Exponential-Linear Activation

`elu()` applies a three-parameter exponential linear unit elementwise:

```text
elu(x) = scale * x                                      when x > 0
         alpha * scale * (exp(input_scale * x) - 1)    when x <= 0
```

The positive branch is linear. The non-positive branch is exponential and,
when `alpha`, `scale`, and `input_scale` are positive, approaches
`-alpha * scale` as `x` moves left. This gives negative inputs a smooth,
non-zero response while leaving positive inputs inexpensive to evaluate.

The three parameters have separate jobs:

| Parameter | Meaning | Default |
| --- | --- | --- |
| `alpha` | Sets the amplitude and asymptote of the negative branch. | `1.0` |
| `scale` | Multiplies both branches and therefore the complete output. | `1.0` |
| `input_scale` | Scales only the negative branch's exponential argument, controlling its curvature. | `1.0` |

The branches always meet in value at zero because the exponential branch is
zero there. Their slopes meet only when `alpha * input_scale == 1`: the left
slope is `alpha * scale * input_scale`, while the right slope is `scale`.
The default satisfies that condition. [`celu()`](celu.md) ties
`input_scale` to `1 / alpha` when that continuously differentiable join is the
contract you want. [`selu()`](selu.md) fixes all relevant coefficients to its
self-normalizing constants.

`elu()` is out of place and shape preserving. Real floating input retains its
dtype. Integral and Boolean input promote to `float32`, because the negative
branch is fractional. The method does not constrain the three coefficients to
be positive; finite negative or zero coefficients are evaluated by the formula
above. Non-finite coefficients propagate through ordinary floating arithmetic.
Complex receivers are not accepted because selecting `x > 0` has no complex
ordering.

The forward computation is represented as one fused activation operation. It
allocates a result on the receiver's platform rather than mutating the source.

## Reverse Mode

For a tracked receiver, the local derivative is

```text
scale                                                   when x > 0
alpha * scale * input_scale * exp(input_scale * x)     when x <= 0
```

At exactly zero, the recorded derivative is the non-positive branch's slope.
That boundary detail is visible whenever `alpha * input_scale != 1`. The
exponential derivative remains connected for higher-order differentiation;
the fused forward does not detach it into a constant table.

## In-Place Form

`elu_()` is the in-place sibling of [`elu()`](elu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `elu_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.elu_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.86466472, 0, 2]
```

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.elu_(0.5);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.43233236, 0, 2]
```

## Examples

Example 1. With all defaults, the negative branch is `exp(x) - 1` and the
positive branch is the identity.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).elu());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.8647, -0.6321, 0.0000, 1.0000, 2.0000]
```

Example 2. Supplying only `alpha` changes the negative amplitude and asymptote;
positive values remain unchanged because `scale` is still one.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).elu(2.0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.7293, -1.2642, 0.0000, 1.0000, 2.0000]
```

Example 3. The full call form separates output scale from negative curvature.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0})
        .elu(2.0, 0.5, 0.25));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.3935, -0.2212, 0.0000, 0.5000, 1.0000]
```

Example 4. Rank is preserved and integer input promotes to `float32`.

```cpp
auto y = pluss::make_series({{-2, 0}, {1, 3}}).elu();
pluss::series::print(std::cout, y);
std::cout << "dtype=" << y.scalar_type() << " dim=" << y.dim()
          << " shape=[" << y.size(0) << ',' << y.size(1) << ']';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡ -0.8647 0.0000 ⎤
⎣  1.0000 3.0000 ⎦
dtype=float32 dim=2 shape=[2,2]
```

Example 5. The chosen parameters make the derivative at zero `0.25`, while
the positive branch has slope `0.5`.

```cpp
auto x = pluss::make_series({-2.0, -1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.elu(2.0, 0.5, 0.25).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.1516, 0.1947, 0.2500, 0.5000]
```

Example 6. A zero `scale` makes both branches and every local derivative zero;
it is a valid, if usually deliberate, parameterization.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, 0.0, 2.0}).elu(3.0, 0.0, 2.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.0000, 0.0000, 0.0000]
```

## See Also

- [`celu()`](celu.md) couples the parameters to keep the slope continuous at
  zero.
- [`selu()`](selu.md) uses fixed self-normalizing coefficients.
- [`relu()`](relu.md) replaces the exponential branch with a hard zero.
