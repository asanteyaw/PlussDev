<!-- pluss-hand-authored -->

# Series::sin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sin() const -> Series
```

## What It Does

Applies the sine transform elementwise.

Each input element is interpreted as an angle in radians. The output is the
sine of that angle, so every finite real input maps into `[-1, 1]`.

This is a pure value transform:

- shape is preserved
- each element is independent
- no broadcasting enters the picture

Because sine is periodic, `sin()` is useful when the cyclic nature of the input
matters more than its raw magnitude.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its sine.

## Gradient

`sin(x)` is differentiable for all real inputs, with derivative `cos(x)`.

That means:

- the backward multiplier is `1` at zero
- it drops to `0` at odd multiples of `pi / 2`
- it becomes negative where cosine is negative

The verified reverse-mode example below reflects that behavior on the current
surface.

## Examples

Example 1. Standard anchor angles show the basic sine cycle.

```cpp
const double pi = 3.14159265358979323846;
pluss::series::print(
    std::cout,
    pluss::make_series({-pi / 2.0, 0.0, pi / 2.0, pi}).sin());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1.0000e+00, 0.0000e+00, 1.0000e+00, 1.2246e-16]
```

Example 2. Matrices keep their layout while values transform elementwise.

```cpp
const double pi = 3.14159265358979323846;
std::cout << pluss::make_series({
    {0.0, pi / 2.0},
    {pi, 3.0 * pi / 2.0},
}).sin();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 1],
  [1.2246468e-16, -1]]
```

Example 3. Reverse-mode sensitivity follows `cos(x)`.

```cpp
const double pi = 3.14159265358979323846;
auto x = pluss::make_series({0.0, pi / 2.0, pi});
x.tracks_slope_(true);

auto out = x.sin();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1.2246468e-16]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 6.123234e-17, -1]
```

Example 4. Non-contiguous views can still be transformed normally.

```cpp
const double pi = 3.14159265358979323846;
auto view = pluss::make_series({
    {0.0, pi / 6.0, pi / 3.0},
    {pi / 2.0, 2.0 * pi / 3.0, 5.0 * pi / 6.0},
}).transpose(0, 1);

std::cout << view.sin();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [0.5, 0.8660254],
  [0.8660254, 0.5]]
```

## Gotchas And Failure Modes

- Inputs are interpreted in radians.
- Exact mathematical zeros at angles such as `pi` can show up as tiny floating
  residues like `1.2246e-16`.
- Periodicity can collapse distinct raw values into the same transformed value.

## Summary

Use `sin()` for radian-valued periodic transforms when phase matters. Expect
shape preservation, bounded outputs, and normal floating approximations at
special angles.

## See Also

- [`Series::cos`](cos.md): phase-shifted companion.
- [`Series::tan`](tan.md): tangent ratio transform.
- [`Series::asin`](asin.md): inverse sine on `[-1, 1]`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sign.html">
    <span class="label">Previous</span>
    <span class="title">sign</span>
  </a>
  <a class="next-link" href="sinh.html">
    <span class="label">Next</span>
    <span class="title">sinh</span>
  </a>
</div>
