<!-- pluss-hand-authored -->

# Series::cos

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::cos() const -> Series
Series& Series::cos_()
```

## What It Does

Applies the cosine transform elementwise.

Each input value is interpreted as an angle in radians. The result is the
cosine of that angle, so every finite real input is mapped into the closed
interval `[-1, 1]`.

The method is often useful in two different situations:

- as a periodic transform on angle-like values
- as a bounded feature transform when you want oscillation rather than growth

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its cosine.

## Gradient

`cos(x)` is differentiable for all real inputs, with derivative `-sin(x)`.

That gives a clean mental model:

- the forward values oscillate between `-1` and `1`
- the backward multiplier is zero at peaks and troughs
- the sign of the sensitivity flips with the sine

The verified reverse-mode example below reflects that behavior on the current
surface.

## In-Place Form

`cos_()` is the in-place sibling of [`cos()`](cos.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0});

auto& ref = x.cos_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.54030231, -0.41614684]
same object: true
```

## Examples

Example 1. Standard anchor angles show the familiar cosine cycle.

```cpp
std::cout << pluss::series::print(
    pluss::make_series({0.0, 1.5707963267948966, 3.1415926535897932, 6.2831853071795865}).cos());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000e+00, 6.1232e-17, -1.0000e+00, 1.0000e+00]
```

Example 2. The transform is shape-preserving on 2D inputs too.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {0.0, 1.5707963267948966},
        {3.1415926535897932, 6.2831853071795865},
    }).cos());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡  1.0000e+00 6.1232e-17 ⎤
⎣ -1.0000e+00 1.0000e+00 ⎦
```

Example 3. Reverse-mode sensitivity follows `-sin(x)`.

```cpp
auto x = pluss::make_series({0.0, 0.5235987755982988, 1.0471975511965976});
x.tracks_slope_(true);

auto out = x.cos();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.8660254, 0.5]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0, -0.5, -0.8660254]
```

Example 4. Cosine is an even function, so symmetric positive and negative
angles produce the same values.

```cpp
std::cout << pluss::make_series({
    -1.0471975511965976,
    -0.5235987755982988,
    0.0,
    0.5235987755982988,
    1.0471975511965976,
}).cos();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.5, 0.8660254, 1, 0.8660254, 0.5]
```

## Gotchas And Failure Modes

- Inputs are interpreted in radians, not degrees.
- Values that are mathematically zero at special angles may show tiny floating
  remainders such as `6.1232e-17`.
- Periodic transforms can make far-apart inputs look similar; preserve the
  original values when phase matters later.

## Summary

Use `cos()` for a bounded periodic transform on radian-valued inputs. The two
main habits are to remember the radian convention and to expect ordinary
floating rounding at special angles.

## See Also

- [`Series::sin`](sin.md): the phase-shifted companion transform.
- [`Series::tan`](tan.md): ratio-style trigonometric transform.
- [`Series::acos`](acos.md): inverse cosine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_nonzero.html">
    <span class="label">Previous</span>
    <span class="title">count_nonzero</span>
  </a>
  <a class="next-link" href="cosh.html">
    <span class="label">Next</span>
    <span class="title">cosh</span>
  </a>
</div>
