<!-- pluss-hand-authored -->

# Series::hardswish

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::hardswish() const
Series& Series::hardswish_() const
```

## A Bounded Gate With A Linear Positive Tail

`hardswish()` transforms every element with the piecewise function

```text
                 0                    when x <= -3
hardswish(x) =   x * (x + 3) / 6      when -3 < x < 3
                 x                    when x >= 3
```

The method combines a value with a cheap, bounded gate. Large negative values
are suppressed, large positive values pass through unchanged, and values in
the transition interval are attenuated by a quadratic curve. Use it when that
specific activation contract is useful. Use [`hardsigmoid()`](hardsigmoid.md)
when the gate itself, rather than the gated value, is the desired result.

The operation is elementwise: rank, shape, element order, and platform are
preserved. It allocates a new contiguous result and does not mutate the
receiver. Floating receivers retain their dtype. Integral and Boolean
receivers are promoted to `float32`, because the middle branch generally
contains fractions. Complex receivers are rejected: the branch tests require
an ordered real value.

The endpoint choices are deliberate. `x == -3` belongs to the zero branch and
`x == 3` belongs to the identity branch. Those choices also determine the
one-sided reverse rule at the two corners.

## Reverse Mode

For a tracked floating receiver, the recorded derivative is

```text
                  0                 when x <= -3
d/dx hardswish =  (2x + 3) / 6      when -3 < x < 3
                  1                 when x >= 3
```

The interior derivative depends on `x`, so higher-order differentiation can
observe nonzero curvature inside the transition interval. At `-3` the selected
derivative is `0`; at `3` it is `1`.

## In-Place Form

`hardswish_()` is the in-place sibling of [`hardswish()`](hardswish.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `hardswish_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-4.0, 0.0, 4.0});
x.hardswish_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 4]
```

## Examples

Example 1. The boundary values show all three branches.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0})
        .hardswish());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[0.0000, 0.0000, -0.3750, 0.0000, 1.1250, 3.0000, 4.0000]
```

Example 2. Higher-rank integer input keeps its shape and promotes to
`float32` before fractional values are formed.

```cpp
auto y = pluss::make_series({{-4, -2, 0}, {1, 3, 4}}).hardswish();
pluss::series::print(std::cout, y);
std::cout << "dtype=" << y.scalar_type()
          << " dim=" << y.dim()
          << " shape=[" << y.size(0) << ',' << y.size(1) << ']';
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0.0000 -0.3333 0.0000 ⎤
⎣ 0.6667  3.0000 4.0000 ⎦
dtype=float32 dim=2 shape=[2,3]
```

Example 3. Summing the output exposes the endpoint and interior derivatives.

```cpp
auto x = pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0});
x.tracks_slope_(true);
x.hardswish().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[0.0000, 0.0000, 0.0000, 0.5000, 1.0000, 1.0000, 1.0000]
```

Example 4. The result can act as a value-dependent gate in a larger
elementwise expression.

```cpp
auto payload = pluss::make_series({10.0, 20.0, 30.0});
auto control = pluss::make_series({-4.0, 0.0, 4.0});
pluss::series::print(std::cout, payload.mul(control.hardswish()));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.0000, 120.0000]
```

Example 5. Complex values have no ordering for the branch boundaries.

```cpp
try {
  using Complex = std::complex<double>;
  (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1})
      .hardswish();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
hardswish: complex input is not supported
```

## See Also

- [`hardsigmoid()`](hardsigmoid.md) returns the bounded gate without multiplying by the receiver.
- [`mish()`](mish.md) is a smooth activation without piecewise corners.
- [`relu()`](relu.md) suppresses negative values without a transition interval.
