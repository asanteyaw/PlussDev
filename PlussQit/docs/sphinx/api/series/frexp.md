<!-- pluss-hand-authored -->

# Series::frexp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::frexp() const
```

## What It Does

Splits each element `x` into a normalized fraction and a power-of-two exponent:

```
x == mantissa * 2^exponent,   with 0.5 <= |mantissa| < 1   (both 0 when x == 0)
```

This is the elementwise form of C's `frexp`. It is the exact inverse of
[`ldexp()`](ldexp.md): `mantissa.ldexp(exponent)` reconstructs `x` bit for bit.
Use it to inspect or rescale the binary magnitude of a series without touching
its significant digits — for example to bring a batch of values into a common
exponent before summing, or to implement a stable log or a custom float format.

The return is a `std::pair<Series, Series>` — a fixed arity of two, so a pair
rather than a single value:

- `.first` is the **mantissa**, same shape and floating dtype as the input
  (`Float32 -> Float32`, `Float64 -> Float64`)
- `.second` is the **exponent**, same shape, dtype `int32`

Structured bindings work: `auto [m, e] = x.frexp();`.

## Parameters

None.

## Returns

`std::pair<Series, Series>`
    `{mantissa, exponent}`. The mantissa keeps the input's floating dtype; the
    exponent is `int32`.

Raises if the input is not a floating-point series.

## Gradient

The mantissa is differentiable within each interval where the exponent remains
constant:

```
d mantissa / d x = 2^(-exponent)
```

The integer exponent is non-differentiable. At a boundary where the selected
exponent changes, the reported sensitivity is the derivative of the local
mantissa branch rather than a derivative through the discontinuous exponent.

## Examples

Example 1. The fraction is always in `[0.5, 1)` in magnitude; the exponent
carries the binary scale.

```cpp
auto [mantissa, exponent] =
    pluss::make_series({1.0, 6.0, 0.75, 0.0, -8.0}).frexp();

pluss::series::print(std::cout, mantissa) << '\n';
pluss::series::print(std::cout, exponent);
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.5000, 0.7500, 0.7500, 0.0000, -0.5000]
Series(shape=[5], strides=[1], dtype=int32, platform=cpu, numel=5, contiguous=true)
data=[1, 3, 0, 0, 4]
```

`1 = 0.5 * 2^1`, `6 = 0.75 * 2^3`, `0.75 = 0.75 * 2^0`, `0` maps to `(0, 0)`,
and `-8 = -0.5 * 2^4`.

Example 2. `frexp()` and [`ldexp()`](ldexp.md) are inverses.

```cpp
auto [m, e] = pluss::make_series({3.0, 100.0, 0.1}).frexp();
pluss::series::print(std::cout, m.ldexp(e));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.0000, 100.0000, 0.1000]
```

Example 3. Reverse through the mantissa uses the exponent selected for each
element.

```cpp
auto x = pluss::make_series({1.0, 6.0, 0.75, -8.0});
x.tracks_slope_(true);
auto [m, e] = x.frexp();
m.sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, 0.1250, 1.0000, 0.0625]
```

Those values are `2^-1`, `2^-3`, `2^0`, and `2^-4`. The sign of the
input does not change the local scale factor.

Example 4. A non-floating input raises.

```cpp
try {
  auto [m, e] = pluss::make_series({1, 2, 3}).frexp();
  pluss::series::print(std::cout, m);
} catch (const std::exception& err) {
  std::string msg = err.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
frexp: floating-point input required
```

## Gotchas And Failure Modes

- The exponent is `int32`, not `int64`.
- `frexp(0)` is `(0, 0)`; the mantissa is not forced onto `[0.5, 1)` for a zero
  input.
- Integer and boolean series raise `frexp: floating-point input required`;
  convert with [`to()`](to.md) first.
- Only the mantissa carries sensitivity; the integer exponent does not.
- The mantissa sign matches the input sign (`frexp(-8) = (-0.5, 4)`), so
  `|mantissa|` is what lies in `[0.5, 1)`.

## Summary

`frexp()` decomposes `x` into `mantissa * 2^exponent`:

- returns `std::pair<Series, Series>` = `{mantissa (float), exponent (int32)}`
- `|mantissa|` in `[0.5, 1)`; `(0, 0)` for a zero input
- the exact inverse of [`ldexp()`](ldexp.md)
- floating-point input only; mantissa differentiable within each exponent band

## See Also

- [`Series::ldexp`](ldexp.md): the inverse — `mantissa * 2^exponent`.
- [`Series::log2`](log2.md): the continuous analogue of the exponent.
- [`Series::sort`](sort.md): another method returning a `std::pair<Series,
  Series>`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frac.html">
    <span class="label">Previous</span>
    <span class="title">frac</span>
  </a>
  <a class="next-link" href="gather.html">
    <span class="label">Next</span>
    <span class="title">gather</span>
  </a>
</div>
