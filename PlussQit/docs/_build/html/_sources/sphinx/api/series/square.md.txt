<!-- pluss-hand-authored -->

# Series::square

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::square() const -> Series
Series& Series::square_()
```

## What It Does

`square()` returns `x * x` elementwise.

It is a named primitive rather than `x.mul(x)` or `x.pow(2.0)` for two reasons:

- it reads the intent directly, and
- its backward is `2 * x`, which is cheaper and more numerically direct than
  routing a general power rule through `pow()`.

Unlike most of the unary math methods, `square()` **preserves the input
dtype**. Squaring an `int64` series gives an `int64` series (subject to integer
overflow), and squaring a `float32` series gives `float32`.

## Parameters

None.

## Returns

`Series` or `Series&`
    `x * x` elementwise, same dtype as the input. `square_()` overwrites the
    current storage.

## Gradient

`square()` is differentiable, with the simplest non-trivial polynomial rule:

```
d/dx square(x) = 2 * x
```

## Examples

Example 1. Squaring is sign-independent, so a symmetric input gives a symmetric
result.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}).square());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[4.0000, 0.2500, 0.0000, 0.2500, 4.0000]
```

Example 2. Integer input stays integer.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2, 3, 4}, pluss::ScalarType::Int64).square());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[4, 9, 16]
```

Example 3. `square()` can overflow. `(1e200)^2` is `1e400`, out of `float64`
range.

```cpp
pluss::series::print(std::cout, pluss::make_series({1e200}).square());
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[inf]
```

Example 4. The backward multiplier is `2 * x`.

```cpp
auto x = pluss::make_series({-2.0, 3.0, 5.0});
x.tracks_slope_(true);

x.square().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-4, 6, 10]
```

## Gotchas And Failure Modes

- `square()` keeps the input dtype, so an integer square can overflow silently
  and a large float square can overflow to `inf`. Cast to a wider type first if
  that is a concern.
- For a squared *magnitude* of a complex series, use [`abs2()`](abs2.md); for
  real input the two agree.
- `NaN` propagates.

## Summary

`square(x)` is `x * x` as a dtype-preserving primitive:

- keeps `int64` / `float32` / `float64` as-is
- differentiable, backward multiplier `2 * x`

## See Also

- [`Series::abs2`](abs2.md): squared magnitude; identical to `square()` for real
  input, `|z|^2` for complex.
- [`Series::sqrt`](sqrt.md): the inverse on non-negative input.
- [`Series::pow`](pow.md): the general power, of which `square()` is the `2`
  case with a cheaper backward.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sqrt.html">
    <span class="label">Previous</span>
    <span class="title">sqrt</span>
  </a>
  <a class="next-link" href="squeeze.html">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>
