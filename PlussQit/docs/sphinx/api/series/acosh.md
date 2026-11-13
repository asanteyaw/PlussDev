<!-- pluss-hand-authored -->

# Series::acosh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::acosh() const
Series& Series::acosh_()
```

## What It Does

`acosh()` applies the inverse hyperbolic cosine elementwise. If `y=acosh(x)`,
then `cosh(y)=x` and the principal result satisfies `y>=0`.

Unlike an ordinary inverse cosine, this method does not return an angle.
Hyperbolic functions parameterize the unit hyperbola; `acosh()` recovers the
nonnegative hyperbolic coordinate whose `cosh()` value is the input.

## Recovering A Nonnegative Hyperbolic Coordinate

Real hyperbolic cosine never falls below one. The real domain is therefore
`x>=1`. The boundary value `acosh(1)=0` is valid, while values below one return
`nan`. This domain is the first condition to check when `acosh()` appears after
roundoff-prone arithmetic.

The operation is elementwise: shape is preserved, dimensions do not interact,
and no reduction or broadcasting occurs.

## Parameters

None.

## Returns

`Series`
    A new shape-preserving `Series`. Floating inputs retain their floating
    dtype. Bool, integral, and float8 inputs promote to a supported floating
    dtype because inverse hyperbolic values are generally fractional.

## Gradient

For `x>1`,

```text
d acosh(x) / dx = 1 / sqrt(x^2 - 1)
```

The derivative is positive and becomes unbounded as `x` approaches `1` from
above. The forward boundary is valid, but it is not a numerically gentle point
for gradient-based work.

## In-Place Form

`acosh_()` is the in-place sibling of [`acosh()`](acosh.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 5.0});

auto& ref = x.acosh_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1.3169579, 2.2924317]
same object: true
```

## Examples

Example 1. Values produced by `cosh()` map back to their nonnegative inputs.

```cpp
auto values = pluss::make_series({
    1.0,
    1.5430806348152437,
    3.7621956910836314,
});
std::cout << values.acosh() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
```

Example 2. The domain boundary and a violation remain visible in the result.

```cpp
std::cout << pluss::make_series({0.5, 1.0, 2.0}).acosh() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, 0, 1.3169579]
```

Example 3. Reverse mode follows the analytic interior derivative.

```cpp
auto x = pluss::make_series({1.25, 2.0});
x.tracks_slope_(true);
x.acosh().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.3333333, 0.57735027]
```

## Gotchas And Failure Modes

- Real inputs below `1` produce `nan`.
- The derivative diverges at the valid boundary `x=1`.
- This is an inverse hyperbolic function, not an angle-producing inverse
  trigonometric function.
- The method is out-of-place; the separate `acosh_()` surface mutates.

## Summary

Use `acosh()` to recover nonnegative hyperbolic coordinates from values in
`[1,infinity)`. Guard the lower domain boundary when gradients matter.

## Aliases

**`arccosh()`** / **`arccosh_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `acosh()` / `acosh_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::cosh`](cosh.md): forward hyperbolic cosine.
- [`Series::asinh`](asinh.md): unrestricted inverse hyperbolic sine.
- [`Series::atanh`](atanh.md): inverse hyperbolic tangent on `[-1,1]`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="acos.html"><span class="label">Previous</span><span class="title">acos</span></a>
  <a class="next-link" href="add.html"><span class="label">Next</span><span class="title">add</span></a>
</div>
