<!-- pluss-hand-authored -->

# Series::besselj

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::besselj() const
Series Series::besselj(double order) const
```

## Oscillatory Cylindrical Solutions

`besselj()` evaluates the Bessel function of the first kind, `J_order(x)`,
elementwise. It is the solution of

```text
x² y'' + x y' + (x² - order²) y = 0
```

that is finite at the origin for non-negative order. It appears when a wave,
diffusion process, or eigenproblem is expressed in cylindrical coordinates.
Use the no-argument form for order zero; it is exactly `besselj(0.0)`. Pass an
order when the radial mode or analytic formula requires another member of the
family. The order is one scalar shared by every lane; it does not broadcast
from another `Series`.

Integer orders are evaluated without perturbing the order, including the
parity relation `J_n(-x) = (-1)^n J_n(x)`. Non-integer orders use the real
power-series branch for non-negative `x`. A negative argument with non-integer
order has no generally real value and returns `NaN`. `order` itself must be
finite.

The operation is out of place and preserves shape and platform. It returns a
real floating `Series`; integral input promotes to `float64`. No reduction or
broadcasting occurs.

## Reverse Mode

The local derivative uses the neighboring-order recurrence

```text
d J_v(x) / dx = (J_(v-1)(x) - J_(v+1)(x)) / 2
```

For order zero this simplifies to `-J_1(x)`. An incoming sensitivity is
multiplied lane-wise by that value.

## Examples

Example 1. The omitted order evaluates `J_0`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besselj());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.9385, 0.7652, 0.2239, -0.3971]
```

The sign change by `x=4` is normal: `J_0` oscillates and crosses zero.

Example 2. Order one selects a different radial mode.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besselj(1.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.2423, 0.4401, 0.5767, -0.0660]
```

Example 3. Fractional order is accepted on the non-negative real branch.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besselj(0.5));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5410, 0.6714, 0.5130, -0.3019]
```

Example 4. Reverse mode for `J_0` produces `-J_1`.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
x.tracks_slope_(true);
x.besselj().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.2423, -0.4401, -0.5767]
```

Example 5. A non-finite order is rejected before lane evaluation.

```cpp
try {
  (void)pluss::make_series({1.0}).besselj(
      std::numeric_limits<double>::infinity());
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
bessel: order must be finite
```

## See Also

- [`bessely()`](bessely.md) is the singular second real solution.
- [`besseli()`](besseli.md) is the exponentially growing modified family.
- [`besselk()`](besselk.md) is the decaying modified family.
