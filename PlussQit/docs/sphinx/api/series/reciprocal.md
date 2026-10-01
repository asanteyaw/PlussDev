<!-- pluss-hand-authored -->

# Series::reciprocal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::reciprocal() const -> Series
Series& Series::reciprocal_()
```

## What It Does

`reciprocal()` applies `1 / x` elementwise.

It is the multiplicative inverse lane by lane. Nothing is reduced, no axis is
chosen, and the output shape matches the input shape exactly.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing `1 / x` lane by
    lane.

## Gradient

If `y = 1 / x`, then:

- `dy/dx = -1 / x²`

In the example below, `L = sum(y)`, so each lane receives a seed of `1` and the
stored sensitivity becomes exactly `-1 / x_i²`.

## In-Place Form

`reciprocal_()` is the in-place sibling of [`reciprocal()`](reciprocal.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 4.0});

auto& ref = x.reciprocal_();
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

Example 1. Each lane is inverted independently.

```cpp
pluss::series::print(std::cout, pluss::make_series({2.0, -4.0, 0.5}).reciprocal());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5000, -0.2500, 2.0000]
```

Example 2. The backward multiplier is negative and scales with the square of
the denominator.

```cpp
auto x = pluss::make_series({2.0, 4.0, -0.5});
x.tracks_slope_(true);

auto y = x.reciprocal();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5, 0.25, -2]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.25, -0.0625, -4]
```

## Gotchas And Failure Modes

- The derivative is negative: increasing `x` decreases `1/x`.
- Small-magnitude inputs can produce large outputs and large sensitivities.
- This is a pure elementwise inverse; it is not matrix inversion.

## Summary

Use `reciprocal()` when each lane should be inverted independently and that
intent should stay explicit in the code.

## See Also

- [`Series::div`](div.md): general division surface.
- [`Series::pow`](pow.md): reciprocal can be viewed as `x^{-1}`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="real.html">
    <span class="label">Previous</span>
    <span class="title">real</span>
  </a>
  <a class="next-link" href="relu.html">
    <span class="label">Next</span>
    <span class="title">relu</span>
  </a>
</div>
