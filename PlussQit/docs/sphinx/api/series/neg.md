<!-- pluss-hand-authored -->

# Series::neg

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::neg() const -> Series
```

## What It Does

Applies unary negation elementwise.

`neg()` does not change magnitude. It flips sign:

- positive values become negative
- negative values become positive
- zero remains zero, though its printed spelling may show a negative zero in
  some floating outputs

This is the method form of unary minus. It is useful when you want the
transform as part of a chain, or when you want to teach the explicit method
before relying on operator shorthand.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    multiplied by `-1`.

## Gradient

`neg()` is as simple as it looks:

- the forward value is `-x`
- the backward multiplier is `-1`

That makes it one of the clearest introductory examples of a transform whose
gradient is constant.

## Examples

Example 1. Basic unary negation on a matrix.

```cpp
auto base = pluss::make_series({
    {-3.0, -2.0, -1.0},
    {0.0, 1.0, 4.0},
});

pluss::series::print(std::cout, base.neg());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  3.0000  2.0000  1.0000 ⎤
⎣ -0.0000 -1.0000 -4.0000 ⎦
```

Example 2. The unary `-` operator is equivalent.

```cpp
std::cout << (-base);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 2, 1],
  [-0, -1, -4]]
```

Example 3. The backward sensitivity is a constant `-1`.

```cpp
auto x = pluss::make_series({-3.0, 0.5, 4.0});
x.tracks_slope_(true);

auto out = x.neg();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, -0.5, -4]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, -1, -1]
```

## Gotchas And Failure Modes

- Floating output may display `-0` or `-0.0000`. That is a printing-level
  consequence of signed zero, not a separate semantic value class.
- `neg()` flips direction but does not otherwise normalize, clamp, or rescale
  anything.

## Summary

Teach `neg()` as the explicit method form of unary minus. It is simple, but it
is also a useful anchor because its derivative is constant and easy to reason
about.

## See Also

- [`Series::abs`](abs.md): discards sign instead of flipping it.
- [`Series::sign`](sign.md): keeps only direction information.
- [`Series::sub`](sub.md): subtraction as a binary operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ne.html">
    <span class="label">Previous</span>
    <span class="title">ne</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
