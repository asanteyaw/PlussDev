<!-- pluss-hand-authored -->

# Series::bessely

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::bessely() const
Series Series::bessely(double order) const
```

## The Singular Cylindrical Solution

`bessely()` evaluates the Bessel function of the second kind,
`Y_order(x)`, elementwise. `Y_v` solves the same cylindrical differential
equation as [`besselj()`](besselj.md), but supplies the independent real
solution that is singular at the origin. It is used when boundary conditions
cannot be represented by `J_v` alone.

The no-argument form means order zero. `bessely(order)` applies one finite
scalar order to every lane. For positive arguments, integer and non-integer
orders are supported. At `x=0`, order-zero `Y` tends to negative infinity. On
the negative real axis, the real-valued branch is undefined and returns `NaN`.
These are mathematical domain results, not exceptions; an invalid non-finite
order is a parameter error and does throw.

Shape and platform are preserved. The result is floating point, with integral
input promoted to `float64`. The transform allocates a result and does not
mutate or broadcast its receiver.

## Reverse Mode

The derivative is evaluated through adjacent orders:

```text
d Y_v(x) / dx = (Y_(v-1)(x) - Y_(v+1)(x)) / 2
```

Sensitivities inherit the same singularity at zero and the same real-domain
restriction on negative arguments.

## Examples

Example 1. Order zero changes sign between the first two positive samples.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).bessely());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.4445, 0.0883, 0.5104, -0.0169]
```

Example 2. Supplying order one selects `Y_1` rather than changing the shape.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).bessely(1.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1.4715, -0.7812, -0.1070, 0.3979]
```

Example 3. Domain boundaries are represented lane by lane.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0}).bessely());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, -inf, 0.0883]
```

Example 4. The order-zero reverse result is `-Y_1(x)`.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
x.tracks_slope_(true);
x.bessely().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.4715, 0.7812, 0.1070]
```

Example 5. Higher-rank input retains its geometry.

```cpp
const auto y = pluss::make_series({{0.5, 1.0}, {2.0, 4.0}}).bessely();
std::cout << "dim=" << y.dim()
          << " shape=[" << y.size(0) << "," << y.size(1) << "]\n";
```

### Output

```text
dim=2 shape=[2,2]
```

## See Also

- [`besselj()`](besselj.md) is finite at the origin for non-negative order.
- [`besselk()`](besselk.md) is the singular but exponentially decaying modified
  solution.
