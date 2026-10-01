<!-- pluss-hand-authored -->

# Series::tan

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tan() const -> Series
Series& Series::tan_()
```

## What It Does

Applies the tangent transform elementwise.

Tangent is the ratio `sin(x) / cos(x)`, so it behaves very differently from
[`Series::sin`](sin.md) and [`Series::cos`](cos.md):

- it is periodic
- it is unbounded
- it becomes extremely steep near odd multiples of `pi / 2`

This makes `tan()` useful when slope-like angular information matters, but it
also makes it much easier to produce numerically large outputs.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its tangent.

## Gradient

Mathematically, `tan(x)` has derivative `sec(x)^2 = 1 / cos(x)^2` wherever the
function is defined.

So the backward multiplier:

- is always non-negative on valid real inputs
- equals `1` at zero
- becomes very large near the vertical asymptote points

On the current verified public surface used here, the reverse-mode example
returns zero sensitivities. So the value transform is verified, but the
backward path through this method is not yet reliable.

## In-Place Form

`tan_()` is the in-place sibling of [`tan()`](tan.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.tan_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1.5574077]
same object: true
```

## Examples

Example 1. Standard anchor angles show the familiar `-1`, `0`, `1` values.

```cpp
const double pi = 3.14159265358979323846;
pluss::series::print(
    std::cout,
    pluss::make_series({-pi / 4.0, 0.0, pi / 4.0}).tan());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.0000, 0.0000, 1.0000]
```

Example 2. Matrices retain their shape while values transform independently.

```cpp
std::cout << pluss::make_series({
    {-0.5, 0.0},
    {0.5, 1.0},
}).tan();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-0.54630249, 0],
  [0.54630249, 1.5574077]]
```

Example 3. The current reverse-mode path yields zero sensitivities on this
surface.

```cpp
auto x = pluss::make_series({-0.5, 0.0, 0.5});
x.tracks_slope_(true);

auto out = x.tan();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.54630249, 0, 0.54630249]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Values near `pi / 2` can become large very quickly even before
reaching the asymptote.

```cpp
std::cout << pluss::make_series({1.2, 1.4, 1.5}).tan();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5721516, 5.7978837, 14.10142]
```

## Gotchas And Failure Modes

- Inputs are interpreted in radians.
- `tan()` becomes numerically steep near odd multiples of `pi / 2`.
- Large outputs do not require large inputs; they can appear simply because the
  input angle is close to an asymptote.
- The current verified backward path returns zero sensitivities here.

## Summary

Use `tan()` when slope-like angular behavior matters, but keep a close eye on
how near your inputs are to the asymptote locations. Forward behavior is
verified; current reverse-mode support on this public path is not.

## See Also

- [`Series::sin`](sin.md): bounded trigonometric companion.
- [`Series::cos`](cos.md): bounded trigonometric companion.
- [`Series::atan`](atan.md): inverse tangent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take_along_dim.html">
    <span class="label">Previous</span>
    <span class="title">take_along_dim</span>
  </a>
  <a class="next-link" href="tanh.html">
    <span class="label">Next</span>
    <span class="title">tanh</span>
  </a>
</div>
