<!-- pluss-hand-authored -->

# Series::besseli

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::besseli() const
Series Series::besseli(double order) const
```

## The Growing Modified Bessel Family

`besseli()` evaluates the modified Bessel function of the first kind,
`I_order(x)`, elementwise. It solves

```text
x² y'' + x y' - (x² + order²) y = 0
```

and is the solution that remains finite at zero for non-negative order. Unlike
the oscillatory `J` and `Y` families, `I_v(x)` grows approximately like
`exp(x) / sqrt(2*pi*x)` on the positive axis. It commonly represents the
growing component of radial diffusion and concentration models.

Calling `besseli()` is identical to `besseli(0.0)`. The explicit overload uses
one finite scalar order for all elements. Integer orders obey
`I_-n(x) = I_n(x)` and have real parity on negative arguments. A negative
argument with non-integer order does not generally have a real value, so the
real-valued API returns `NaN` for that lane.

The transform is out of place and shape preserving. Floating input produces a
floating result; integral input promotes to `float64`. Growth is genuine: a
large positive argument may overflow to positive infinity rather than being
silently scaled. Use a logarithmic formulation in algorithms whose arguments
can enter that regime.

## Reverse Mode

The derivative uses the sum of adjacent orders:

```text
d I_v(x) / dx = (I_(v-1)(x) + I_(v+1)(x)) / 2
```

For order zero, symmetry reduces this to `I_1(x)`.

## Examples

Example 1. The default order demonstrates monotone positive-axis growth.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besseli());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0635, 1.2661, 2.2796, 11.3019]
```

Example 2. Order one is zero at the origin and grows with a different near-zero
power law.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besseli(1.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.2579, 0.5652, 1.5906, 9.7595]
```

Example 3. Integer-order parity permits negative arguments. `I_0` is even and
`I_1` is odd.

```cpp
const auto x = pluss::make_series({-1.0, 0.0, 1.0});
pluss::series::print(std::cout, x.besseli());
pluss::series::print(std::cout, x.besseli(1.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.2661, 1.0000, 1.2661]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5652, 0.0000, 0.5652]
```

Example 4. Reverse mode for order zero returns `I_1`.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
x.tracks_slope_(true);
x.besseli().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.2579, 0.5652, 1.5906]
```

Example 5. A scalar remains rank zero.

```cpp
const auto y = pluss::make_series(0.0).besseli();
std::cout << "dim=" << y.dim() << " value=" << y.item<double>() << "\n";
```

### Output

```text
dim=0 value=1
```

## See Also

- [`besselk()`](besselk.md) supplies the decaying modified solution.
- [`besselj()`](besselj.md) supplies the corresponding oscillatory family.
