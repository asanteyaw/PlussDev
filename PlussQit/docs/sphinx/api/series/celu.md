<!-- pluss-hand-authored -->

# Series::celu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::celu(double alpha = 1.0) const
Series& Series::celu_() const
Series& Series::celu_(double alpha) const
```

## Continuously Differentiable Exponential Linear Unit

`celu()` applies the continuously differentiable exponential linear unit
elementwise:

```text
celu(x, alpha) = x                              when x > 0
                 alpha * (exp(x / alpha) - 1)  otherwise
```

The positive branch is the identity. With positive `alpha`, the negative
branch is exponential and approaches `-alpha`. Both branches meet at zero with
value zero and slope one. That smooth join is the reason this operation exists:
it retains an exponential negative response without introducing a derivative
jump at the origin.

`alpha` controls both the negative saturation level and the curvature. The
default gives `exp(x)-1` on the non-positive side. A larger positive value
makes that side less sharply curved. Zero is invalid because the formula
divides by `alpha`; validation occurs before an output is returned. A finite
negative value is evaluated by the same formula, although a positive value is
the conventional activation contract.

The operation is out of place and preserves shape and platform. Floating input
keeps its dtype. Integral input promotes to `float32`; the fractional negative
branch is never truncated into integer storage.

## Reverse Mode

For a tracked input, the local derivative is

```text
1                 when x > 0
exp(x / alpha)    when x <= 0
```

The recorded derivative at zero is one. The backward path is represented as
one activation node rather than exposing the comparison and exponential used
to evaluate its branches.

## In-Place Form

`celu_()` is the in-place sibling of [`celu()`](celu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `celu_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.celu_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.86466472, 0, 2]
```

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.celu_(2.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.2642411, 0, 2]
```

## Examples

Example 1. The default leaves positive values unchanged and bends negative
values toward `-1`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).celu());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.8647, -0.6321, 0.0000, 1.0000, 2.0000]
```

Example 2. `alpha=2` changes only the non-positive branch.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).celu(2.0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.2642, -0.7869, 0.0000, 1.0000, 2.0000]
```

Example 3. Rank and shape survive while integer input promotes.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{-2, 0}, {1, 3}}).celu(0.5));
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=
⎡ -0.4908 0.0000 ⎤
⎣  1.0000 3.0000 ⎦
```

Example 4. A sum objective reveals the elementwise derivative.

```cpp
auto x = pluss::make_series({-2.0, -1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.celu(2.0).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.3679, 0.6065, 1.0000, 1.0000]
```

Example 5. Zero `alpha` is rejected explicitly.

```cpp
try {
  (void)pluss::make_series({-1.0, 1.0}).celu(0.0);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
celu: expected alpha != 0
```

## See Also

- [`elu()`](elu.md) exposes the general exponential-linear parameters.
- [`selu()`](selu.md) uses fixed self-normalizing coefficients.
- [`relu()`](relu.md) uses a hard zero branch instead of an exponential one.
