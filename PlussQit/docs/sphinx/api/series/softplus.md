<!-- pluss-hand-authored -->

# Series::softplus

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::softplus() const
Series Series::softplus(double beta, double threshold) const
Series& Series::softplus_() const
```

## What It Does

`softplus()` is a smooth approximation of [`relu()`](relu.md):

```
softplus(x) = (1 / beta) * log(1 + exp(beta * x))
```

With the default `beta = 1` it is `log(1 + exp(x))`. The curve is always
positive, is `log(2)` at `x = 0`, decays smoothly toward `0` for large negative
`x`, and approaches the line `y = x` for large positive `x`. Increasing `beta`
sharpens the bend so the shape converges on `relu()`.

For numerical safety, once `beta * x` exceeds `threshold` (default `20`) the
function returns `x` directly — `exp(beta * x)` past that point is so large that
`(1/beta) * log(1 + exp(beta * x))` equals `x` to full precision anyway.

`softplus()` needs a floating-point input.

## Parameters

**beta** (`double`, default `1`)

Sharpness. Larger values make the transition tighter around `x = 0`.

**threshold** (`double`, default `20`)

Above `beta * x = threshold` the output is `x` unchanged, to avoid overflow in
`exp`.

## Returns

`Series`
    The activated values, always positive, same shape and floating-point dtype
    as the input.

## Gradient

`softplus()` is differentiable, and its derivative is a (scaled) sigmoid:

```
d/dx softplus(x) = sigmoid(beta * x)
```

With `beta = 1` that is exactly `sigmoid(x)` — `0.5` at the origin, tending to
`1` on the right and `0` on the left. Unlike `relu()`, the gradient is never
exactly `0`, so a `softplus()` unit stays trainable everywhere.

## In-Place Form

`softplus_()` is the in-place sibling of [`softplus()`](softplus.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `softplus_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.softplus_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.69314718, 1.3132617]
same object: true
```

## Examples

Example 1. The default `beta = 1` form, `log(1 + exp(x))`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).softplus());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0486, 0.3133, 0.6931, 1.3133, 3.0486]
```

`softplus(0) = log(2) = 0.6931`, and by `x = 3` the output is already close to
`x`.

Example 2. A larger `beta` sharpens the bend toward `relu()`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).softplus(2.0, 20.0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0012, 0.0635, 0.3466, 1.0635, 3.0012]
```

At `x = 0` the value is now `log(2) / 2 = 0.3466`, and the negative tail is
pushed closer to `0`.

Example 3. Past the threshold the output is exactly `x`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({30.0, 50.0}).softplus());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[30.0000, 50.0000]
```

Example 4. The backward is `sigmoid(beta * x)`.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
x.tracks_slope_(true);

x.softplus().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0474, 0.2689, 0.5000, 0.7311, 0.9526]
```

Those are `sigmoid([-3, -1, 0, 1, 3])`.

## Gotchas And Failure Modes

- The two-argument form takes `beta` **and** `threshold` together; there is no
  `beta`-only overload.
- Floating-point input only; an integer series raises.
- The output is strictly positive, so `softplus()` is not a drop-in for
  `relu()` where an exact `0` is required.

## Summary

`softplus(x)` is a smooth, strictly positive `relu()`:

- `(1/beta) * log(1 + exp(beta * x))`, returning `x` once `beta * x > threshold`
- floating-point input only
- differentiable, backward `sigmoid(beta * x)` — never exactly `0`

## See Also

- [`Series::relu`](relu.md): the non-smooth limit as `beta -> inf`.
- [`Series::sigmoid`](sigmoid.md): the derivative of `softplus()`.
- [`Series::logaddexp`](logaddexp.md): `softplus(x)` is `logaddexp(x, 0)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="softmax.html">
    <span class="label">Previous</span>
    <span class="title">softmax</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
