<!-- pluss-hand-authored -->

# Series::cbrt

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::cbrt() const -> Series
Series& Series::cbrt_()
```

## What It Does

`cbrt()` is the real cube root, applied elementwise.

The key difference from `pow(x, 1.0/3.0)` is the negative branch. Cube root is
defined for every real number — `cbrt(-8) = -2`, because `(-2)^3 = -8` — but
`pow()` with a fractional exponent treats a negative base as a domain error and
returns `NaN`. `cbrt()` gives the real root across the whole line.

An integer input is promoted to a floating-point result.

## Parameters

None.

## Returns

`Series` or `Series&`
    The real cube root elementwise, defined for negatives as well as
    non-negatives. `cbrt_()` overwrites the current storage.

## Gradient

`cbrt()` is differentiable. Writing `y = cbrt(x)`:

```
d/dx cbrt(x) = 1 / (3 * x^(2/3)) = 1 / (3 * y^2)
```

The curve has a vertical tangent at `x = 0`, so the derivative there is `+inf`.
Everywhere else it is a positive, decreasing function of `|x|`.

## Examples

Example 1. Cube root of a negative, zero, and a positive.

```cpp
pluss::series::print(std::cout, pluss::make_series({-8.0, 0.0, 27.0}).cbrt());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2.0000, 0.0000, 3.0000]
```

`pow(-8.0, 1.0/3.0)` would give `NaN` for the first entry; `cbrt()` gives
`-2`.

Example 2. The backward multiplier is `1 / (3 * y^2)`.

```cpp
auto x = pluss::make_series({1.0, 8.0, 27.0});
x.tracks_slope_(true);

x.cbrt().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.33333333, 0.083333333, 0.037037037]
```

For `y = 1, 2, 3` the derivative is `1/3`, `1/12`, `1/27`.

Example 3. At `x = 0` the tangent is vertical, so the gradient is `+inf`.

```cpp
auto x = pluss::make_series({0.0, 1.0});
x.tracks_slope_(true);

x.cbrt().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[inf, 0.33333333]
```

## Gotchas And Failure Modes

- The gradient at `x = 0` is `+inf`. A tracked `cbrt()` on values that can reach
  `0` produces non-finite sensitivities there.
- The result is floating point even for an integer input.
- `NaN` propagates.

## Summary

`cbrt(x)` is the real cube root:

- defined for negatives, unlike `pow(x, 1.0/3.0)`
- promotes integer input to floating point
- differentiable, backward multiplier `1 / (3 * cbrt(x)^2)`, `+inf` at `0`

## See Also

- [`Series::pow`](pow.md): the general power, which returns `NaN` for a negative
  base with a fractional exponent.
- [`Series::sqrt`](sqrt.md): the square root, defined only on non-negative
  input.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cartprod.html">
    <span class="label">Previous</span>
    <span class="title">cartprod</span>
  </a>
  <a class="next-link" href="cauchy_.html">
    <span class="label">Next</span>
    <span class="title">cauchy_</span>
  </a>
</div>
