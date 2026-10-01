<!-- pluss-hand-authored -->

# Series::exp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::exp() const -> Series
Series& Series::exp_()
```

## What It Does

Applies the natural exponential function elementwise.

Each output element is `e^x` for the corresponding input element `x`. This is
an out-of-place unary transform: shape is preserved, no broadcasting is
involved, and the result contains the transformed values in a fresh `Series`.

`exp()` grows quickly. That matters both numerically and conceptually:

- negative inputs map into values between `0` and `1`
- zero maps to `1`
- positive inputs grow rapidly

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its natural exponential.

## Gradient

`exp()` is one of the cleanest differentiable transforms in the API:

- the forward value is `exp(x)`
- the backward multiplier is also `exp(x)`

So the output itself is the local derivative. That makes `exp()` a very useful
method for seeing how a forward transform and its reverse-mode sensitivity can
line up exactly.

## In-Place Form

`exp_()` is the in-place sibling of [`exp()`](exp.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.exp_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2.7182818]
same object: true
```

## Examples

Example 1. A small 1D example shows the familiar anchor values.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0}).exp());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.3679, 1.0000, 2.7183]
```

Example 2. The transform is shape-preserving on matrices too.

```cpp
std::cout << pluss::make_series({
    {0.0, 1.0},
    {2.0, 3.0},
}).exp();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2.7182818],
  [7.3890561, 20.085537]]
```

Example 3. The local derivative matches the output values elementwise.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);

auto out = x.exp();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.36787944, 1, 2.7182818]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.36787944, 1, 2.7182818]
```

Example 4. Very large positive inputs do not throw here; they overflow into
very large finite values and then `inf`.

```cpp
std::cout << pluss::make_series({700.0, 710.0}).exp();
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0142321e+304, inf]
```

## Gotchas And Failure Modes

- `exp()` can overflow on large positive inputs.
- Large negative inputs do not fail, but they collapse toward zero.
- Because the gradient equals the output, the same explosive growth that affects
  values also affects sensitivities.

## Summary

Use `exp()` as a pure shape-preserving value transform whose derivative is
the function itself. That gives students one clean mental model for both the
forward pass and the backward pass.

## See Also

- [`Series::log`](log.md): the natural-log counterpart.
- [`Series::sqrt`](sqrt.md): another differentiable unary transform with a
  domain-sensitive derivative.
- [`Series::abs`](abs.md): non-exponential unary value transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="erfinv.html">
    <span class="label">Previous</span>
    <span class="title">erfinv</span>
  </a>
  <a class="next-link" href="exp2.html">
    <span class="label">Next</span>
    <span class="title">exp2</span>
  </a>
</div>
