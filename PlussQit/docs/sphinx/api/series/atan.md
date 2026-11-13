<!-- pluss-hand-authored -->

# Series::atan

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::atan() const
Series& Series::atan_()
```

## What It Does

Applies the inverse tangent elementwise.

Unlike [`Series::asin`](asin.md) and [`Series::acos`](acos.md), `atan()` has no
finite real-input domain restriction. Every finite real input maps to a finite
principal angle in `(-pi / 2, pi / 2)`, and very large magnitudes saturate
toward those horizontal angle limits rather than growing without bound.

That makes `atan()` useful when you want an angle-like compression of an
unbounded real signal.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its principal inverse tangent in radians.

## Gradient

Mathematically, `atan(x)` has derivative `1 / (1 + x^2)`.

So the derivative:

- is always positive
- equals `1` at zero
- shrinks toward `0` as `|x|` grows

On the current verified public surface used for these examples, the reverse-mode
example below returns zero sensitivities. So the forward transform is verified,
but backward support is not yet reliable through this method path.

## In-Place Form

`atan_()` is the in-place sibling of [`atan()`](atan.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});

auto& ref = x.atan_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.78539816, 0, 0.78539816]
same object: true
```

## Examples

Example 1. Small signed inputs map to the expected principal angles.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0}).atan());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.7854, 0.0000, 0.7854]
```

Example 2. Matrices retain their layout while the values become angles.

```cpp
std::cout << pluss::make_series({
    {-2.0, -0.5},
    {0.5, 2.0},
}).atan();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.1071487, -0.46364761],
  [0.46364761, 1.1071487]]
```

Example 3. The current reverse-mode path yields zero sensitivities on this
surface.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);

auto out = x.atan();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.78539816, 0, 0.78539816]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Large magnitudes compress toward `+-pi / 2` instead of growing
without bound.

```cpp
std::cout << pluss::make_series({-100.0, -10.0, 10.0, 100.0}).atan();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1.5607967, -1.4711277, 1.4711277, 1.5607967]
```

## Gotchas And Failure Modes

- `atan()` does not have the `[-1, 1]` domain restriction of `asin()` and
  `acos()`, but it still compresses information heavily at large magnitudes.
- Distinct large inputs can end up looking numerically similar after the
  transform because of that saturation.
- The current verified backward path returns zero sensitivities here.

## Summary

Use `atan()` when you need an angle-like, bounded view of an unbounded real
signal. The forward behavior is smooth and stable, while current reverse-mode
support on this public path should be treated cautiously.

## Aliases

**`arctan()`** / **`arctan_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `atan()` / `atan_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::tan`](tan.md): forward tangent transform.
- [`Series::asin`](asin.md): inverse sine with a tighter domain.
- [`Series::acos`](acos.md): inverse cosine with a tighter domain.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asinh.html">
    <span class="label">Previous</span>
    <span class="title">asinh</span>
  </a>
  <a class="next-link" href="atan2.html">
    <span class="label">Next</span>
    <span class="title">atan2</span>
  </a>
</div>
