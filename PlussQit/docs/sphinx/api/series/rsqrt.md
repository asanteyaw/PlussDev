<!-- pluss-hand-authored -->

# Series::rsqrt

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::rsqrt() const -> Series
Series& Series::rsqrt_()
Series& Series::rsqrt_() const
```

## What It Does

`rsqrt()` is the reciprocal square root, `1 / sqrt(x)`, applied elementwise.

It is a primitive rather than `sqrt()` followed by [`reciprocal()`](reciprocal.md)
because that pairing is common enough — normalizing a vector by its length,
scaling by an inverse standard deviation, the `1/sqrt(...)` factor in attention
and in many normalization layers — that fusing it into one op saves a pass over
the data and one rounding step.

Domain notes:

- `x > 0` gives an ordinary positive result
- `x = 0` gives `+inf`
- `x < 0` gives `NaN` (the real square root is undefined there)

An integer input is promoted to a floating-point result; a `float32` input
stays `float32`.

## Parameters

None.

## Returns

`Series` or `Series&`
    `1 / sqrt(x)` elementwise. `rsqrt_()` overwrites the current storage.

## Gradient

`rsqrt()` is differentiable. Writing `y = rsqrt(x)`:

```
d/dx rsqrt(x) = -0.5 * x^(-3/2) = -0.5 * y^3
```

The derivative is always negative (a larger `x` gives a smaller `1/sqrt(x)`)
and, like the forward value, it blows up as `x` approaches `0`.

## In-Place Form

`rsqrt_()` is the in-place sibling of [`rsqrt()`](rsqrt.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `rsqrt_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 4.0, 16.0});

auto& ref = x.rsqrt_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.5, 0.25]
same object: true
```

## Examples

Example 1. Perfect squares invert to simple fractions.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 4.0, 9.0, 0.25}).rsqrt());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 0.5000, 0.3333, 2.0000]
```

`1/sqrt(4) = 0.5`, `1/sqrt(9) = 1/3`, and `1/sqrt(0.25) = 1/0.5 = 2`.

Example 2. The domain edges. `0` gives `+inf`, a negative gives `NaN`.

```cpp
pluss::series::print(std::cout, pluss::make_series({0.0, -1.0}).rsqrt());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[inf, nan]
```

Example 3. `float32` in, `float32` out.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({4.0f, 9.0f}, pluss::ScalarType::Float32).rsqrt());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0.5000, 0.3333]
```

Example 4. The backward multiplier is `-0.5 * x^(-3/2)`.

```cpp
auto x = pluss::make_series({1.0, 4.0, 9.0});
x.tracks_slope_(true);

x.rsqrt().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5, -0.0625, -0.018518519]
```

At `x = 1` the derivative is `-0.5`; at `x = 4` it is `-0.5 / 8 = -0.0625`; at
`x = 9` it is `-0.5 / 27`.

## Gotchas And Failure Modes

- `rsqrt(0)` is `+inf`, not an error. A tracked `rsqrt()` on values that can
  reach `0` produces non-finite sensitivities there.
- Negative inputs give `NaN`.
- The result is floating point even for an integer input.

## Summary

`rsqrt(x)` is a fused `1 / sqrt(x)`:

- `+inf` at `0`, `NaN` for negatives
- promotes integer input to floating point; keeps `float32`
- differentiable, backward multiplier `-0.5 * x^(-3/2)`

## See Also

- [`Series::sqrt`](sqrt.md): the square root this inverts.
- [`Series::reciprocal`](reciprocal.md): the `1/x` half; `rsqrt()` fuses it with
  `sqrt()`.
- [`Series::square`](square.md): the sum of squares whose `rsqrt()` is the usual
  normalization scale.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rot90.html">
    <span class="label">Previous</span>
    <span class="title">rot90</span>
  </a>
  <a class="next-link" href="scatter.html">
    <span class="label">Next</span>
    <span class="title">scatter</span>
  </a>
</div>
