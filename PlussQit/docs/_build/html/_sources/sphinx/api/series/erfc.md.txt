<!-- pluss-hand-authored -->

# Series::erfc

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::erfc() const -> Series
Series& Series::erfc_()
Series& Series::erfc_()
```

## What It Does

Applies the complementary error function elementwise:

```
erfc(x) = 1 - erf(x)
```

That identity is the whole idea, but the reason `erfc()` exists as its own
primitive rather than as a two-line composition is precision.

When `x` is large and positive, `erf(x)` is extremely close to `1`. In
floating point, `1 - erf(x)` then subtracts two nearly equal numbers, and the
small true answer is swamped by rounding error in `erf(x)`. Past roughly
`x = 6` in `float64`, `1 - erf(x)` collapses to exactly `0`. The real value of
`erfc(6)` is about `2.15e-17`, which is a perfectly representable `double`;
`erfc()` computes it directly and keeps it.

So the mental model is:

- `erfc(x)` and `1 - erf(x)` agree closely for moderate `x`
- for `x` in the right tail, only `erfc(x)` stays accurate
- for `x` in the left tail, `erfc(x)` approaches `2` and `erf(x)` is the one to
  trust

Properties that follow from the definition and from `erf` being odd:

- `erfc(0) = 1`
- `erfc(-x) = 2 - erfc(x)`
- `erfc(x) -> 0` as `x -> +inf`, and `erfc(x) -> 2` as `x -> -inf`
- `erfc` is strictly decreasing

The tail connection to the normal distribution is the usual reason to use it.
The probability that a standard normal draw exceeds a threshold `t` is
`0.5 * erfc(t / sqrt(2))`, and that formula stays accurate for large `t` in a
way that `1 - Phi(t)` does not.

`erfc()` is an out-of-place unary transform: shape, dtype, and platform are
preserved. `erfc_()` overwrites the current storage in place.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `erfc(x)`. `erfc_()` returns a reference to the mutated current series.

The result dtype matches the input floating-point dtype.

## Gradient

`erfc()` is differentiable. Since `erfc(x) = 1 - erf(x)` and the constant drops
out, its derivative is exactly the negative of the `erf` derivative:

```
d/dx erfc(x) = -(2 / sqrt(pi)) * exp(-x^2)
```

The constant `2 / sqrt(pi)` is `1.1283791670955126`. The backward multiplier is
therefore most negative at `x = 0` and decays like a Gaussian in both
directions. As with `erf()`, a saturated `erfc()` in either tail passes almost
no sensitivity back to its input.

## In-Place Form

`erfc_()` is the in-place sibling of [`erfc()`](erfc.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.erfc_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 0.15729921]
same object: true
```

## Examples

Example 1. The basic decreasing shape, crossing `1` at `x = 0`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).erfc());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.9953, 1.8427, 1.0000, 0.1573, 0.0047]
```

The left tail rises toward `2`, the right tail falls toward `0`, and the
midpoint is exactly `1`.

Example 2. The defining identity holds elementwise: `erf(x) + erfc(x) = 1`.

```cpp
auto x = pluss::make_series({-1.5, -0.3, 0.0, 0.7, 2.2});

pluss::series::print(std::cout, x.erf().add(x.erfc()));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
```

Example 3. Why `erfc()` is a primitive. In the right tail it keeps digits that
`1 - erf(x)` has already lost.

```cpp
auto x = pluss::make_series({5.0, 6.0});

std::cout << "erfc(x)      = ";
pluss::series::print(std::cout, x.erfc());
std::cout << "\n1 - erf(x)   = ";
pluss::series::print(std::cout, pluss::make_series({1.0, 1.0}).sub(x.erf()));
```

### Output

```text
erfc(x)      = Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.5375e-12, 2.1520e-17]
1 - erf(x)   = Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.5374e-12, 0.0000e+00]
```

At `x = 5` the two disagree in the last digits. At `x = 6` the subtraction has
collapsed to `0` while `erfc()` still reports `2.15e-17`.

Example 4. Elementwise and shape-preserving on a matrix. The bottom-right
entry is already in the deep tail.

```cpp
std::cout << pluss::make_series({
    {0.0, 1.0},
    {2.0, 3.0},
}).erfc();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0.15729921],
  [0.004677735, 2.2090497e-05]]
```

Example 5. Non-finite inputs follow the limits. `NaN` propagates, `+inf` maps
to `0`, and `-inf` maps to `2`.

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
double inf = std::numeric_limits<double>::infinity();

pluss::series::print(std::cout, pluss::make_series({nan, inf, -inf}).erfc());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, 0.0000, 2.0000]
```

Example 6. The backward multiplier is the negative of the `erf` rule.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
x.tracks_slope_(true);

auto y = x.erfc();
y.sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.4151075, -1.1283792, -0.87878258, -0.4151075]
```

Compare with the same reverse pass through [`erf()`](erf.md): identical
magnitudes, flipped sign.

Example 7. `erfc()` is floating-point only. Integer and boolean inputs raise
rather than promote.

```cpp
try {
  pluss::series::print(std::cout, pluss::make_series({0, 1, 2}).erfc());
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
erfc: dtype not implemented (float-only)
```

Example 8. `erfc_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({-1.0, 0.0, 1.0});
a.erfc_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.8427, 1.0000, 0.1573]
```

## Gotchas And Failure Modes

- `erfc()` requires a floating-point input. Integer and boolean series raise
  `erfc: dtype not implemented (float-only)`; call [`to()`](to.md) first.
- Reach for `erfc(x)` instead of `1 - erf(x)` whenever `x` can be large and
  positive. The naive subtraction loses precision and eventually returns `0`.
- Symmetrically, in the left tail `erfc(x)` approaches `2` and loses relative
  precision there; for large negative `x` compute with `erf(x)` or with
  `erfc(-x)`.
- The output range is `(0, 2)`. Large-magnitude inputs saturate rather than
  fail.
- Gradients vanish in both tails because the multiplier decays like
  `exp(-x^2)`.
- `NaN` propagates unchanged.

## Summary

`erfc()` is `1 - erf(x)` computed as a primitive so the right tail stays
accurate:

- strictly decreasing, bounded in `(0, 2)`, equal to `1` at `x = 0`
- the accurate choice for Gaussian tail probabilities
- differentiable with backward multiplier `-(2 / sqrt(pi)) * exp(-x^2)`
- floating-point only; convert integral inputs with [`to()`](to.md)
- `erfc_()` is the in-place form

## See Also

- [`Series::erf`](erf.md): the error function itself; `erfc()` is its
  complement and its backward rule is the sign flip of this one.
- [`Series::exp`](exp.md): the primitive the backward rule is built from.
- [`Series::log1p`](log1p.md): another method that exists specifically to keep
  precision that the naive composition would lose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="erf.html">
    <span class="label">Previous</span>
    <span class="title">erf</span>
  </a>
  <a class="next-link" href="erfcinv.html">
    <span class="label">Next</span>
    <span class="title">erfcinv</span>
  </a>
</div>
