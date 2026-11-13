<!-- pluss-hand-authored -->

# Series::cosh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cosh() const
Series& Series::cosh_()
```

## What It Does

Applies the hyperbolic cosine elementwise.

`cosh()` is the even hyperbolic partner to [`Series::sinh`](sinh.md). For all
real inputs, the output is real and never drops below `1`. Negative and
positive inputs of the same magnitude produce the same result.

So the big behavioral difference from ordinary cosine is this:

- `cos()` oscillates between `-1` and `1`
- `cosh()` grows outward from a minimum of `1`

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its hyperbolic cosine.

## Gradient

Mathematically, `cosh(x)` has derivative `sinh(x)`.

So:

- the derivative is negative for negative inputs
- zero at `x = 0`
- positive for positive inputs

On the current verified public surface used here, the reverse-mode example
below returns zero sensitivities. So the forward values are verified, but the
current backward path should not be assumed to be active for this method.

## In-Place Form

`cosh_()` is the in-place sibling of [`cosh()`](cosh.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.cosh_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1.5430806]
same object: true
```

## Examples

Example 1. Small anchor values show the minimum at zero and symmetry around it.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0}).cosh());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5431, 1.0000, 1.5431]
```

Example 2. Matrices keep their shape exactly.

```cpp
std::cout << pluss::make_series({
    {-1.0, -0.5},
    {0.5, 1.0},
}).cosh();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.5430806, 1.127626],
  [1.127626, 1.5430806]]
```

Example 3. The current reverse-mode path yields zero sensitivities on this
surface.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);

auto out = x.cosh();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5430806, 1, 1.5430806]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. `cosh()` stays non-negative and in fact never dips below `1`.

```cpp
std::cout << pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).cosh();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10.067662, 1.5430806, 1, 1.5430806, 10.067662]
```

## Gotchas And Failure Modes

- `cosh()` is not bounded the way `cos()` is; it grows quickly with `|x|`.
- Large magnitudes can become numerically large very fast.
- The current verified reverse-mode path returns zero sensitivities here.

## Summary

Use `cosh()` for the even hyperbolic transform: minimum at zero, same value for
`x` and `-x`, and rapid outward growth. Forward behavior is clear and verified;
current backward support on this public path is not.

## See Also

- [`Series::sinh`](sinh.md): odd hyperbolic companion.
- [`Series::tanh`](tanh.md): bounded hyperbolic ratio transform.
- [`Series::cos`](cos.md): bounded oscillatory cosine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cos.html">
    <span class="label">Previous</span>
    <span class="title">cos</span>
  </a>
  <a class="next-link" href="cov.html">
    <span class="label">Next</span>
    <span class="title">cov</span>
  </a>
</div>
