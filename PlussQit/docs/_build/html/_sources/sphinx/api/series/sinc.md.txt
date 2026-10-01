<!-- pluss-hand-authored -->

# Series::sinc

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sinc() const
Series& Series::sinc_()
Series& Series::sinc_() const
```

## What It Does

Computes the normalized sinc function elementwise:

```
sinc(x) = sin(pi x) / (pi x)   for x != 0,   sinc(0) = 1
```

The `pi` inside is what "normalized" means: the zero crossings land on the
non-zero integers, and the function integrates to `1` over the whole line.
`sinc` is the impulse response of an ideal low-pass filter and the continuous
interpolation kernel behind Whittaker-Shannon reconstruction, so `sinc()` is
the building block for resampling and band-limited interpolation.

Behaviour:

- `sinc(0) = 1`. The `0/0` at the origin is a removable singularity and the
  limit value is filled in.
- `sinc(n) = 0` for every non-zero integer `n` (in floating point, a value
  within rounding of `0`).
- `sinc` is even: `sinc(-x) = sinc(x)`.
- It decays like `1/(pi x)`, oscillating, with the first and largest side lobe
  near `x = 1.43`, reaching about `-0.217`.
- It is smooth and finite everywhere; no poles.

`sinc()` is out-of-place; `sinc_()` overwrites the current storage.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `sinc(x)`. `sinc_()` returns a reference to the mutated series.

An integral input is promoted to `float64`; every non-zero integer maps to a
value within rounding of `0`.

## Gradient

`sinc()` is differentiable everywhere, including at the origin. Away from `0`:

```
d/dx sinc(x) = ( pi x cos(pi x) - sin(pi x) ) / ( pi x^2 )
```

At `x = 0` the derivative is `0` — `sinc` has a smooth maximum there — and the
backward pass returns exactly `0` for that element rather than dividing by
zero.

## In-Place Form

`sinc_()` is the in-place sibling of [`sinc()`](sinc.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `sinc_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.sinc_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 3.8981718e-17]
same object: true
```

## Examples

Example 1. `sinc(0) = 1`; the integers are zeros; `sinc(0.5) = 2/pi`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0}).sinc()
) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000e+00, 6.3662e-01, 3.8982e-17, -2.1221e-01, -3.8982e-17]
```

`sinc(1)` and `sinc(2)` are `~4e-17`, floating-point zero. `sinc(1.5) =
-1/(1.5 pi) = -0.2122`.

Example 2. `sinc` is even.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({-1.5, -0.5, 0.5, 1.5}).sinc());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.2122, 0.6366, 0.6366, -0.2122]
```

Example 3. The backward pass. The slope is exactly `0` at the origin and
follows `(pi x cos(pi x) - sin(pi x)) / (pi x^2)` elsewhere.

```cpp
auto x = pluss::make_series({0.0, 0.5, 1.0, 1.5});
x.tracks_slope_(true);
x.sinc().sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, -1.2732, -1.0000, 0.1415]
```

At `x = 1`, `cos(pi) = -1` and `sin(pi) = 0`, so the slope is `-pi/pi = -1`.

Example 4. `sinc_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({0.0, 0.5, 1.0});
a.sinc_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000e+00, 6.3662e-01, 3.8982e-17]
```

Example 5. The operation is elementwise, so a grid keeps its two-dimensional
layout. This is useful when a table holds sampling offsets for several output
locations: each cell becomes one interpolation weight without flattening the
table.

```cpp
auto offsets = pluss::make_series({{0.0, 0.25}, {0.5, 1.0}});
pluss::series::print(std::cout, offsets.sinc()) << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000e+00 9.0032e-01 ⎤
⎣ 6.3662e-01 3.8982e-17 ⎦
```

Example 6. Out-of-place evaluation can start from integer sample offsets. The
result promotes to `float64`, preserving the small floating-point residuals at
the mathematical zeros instead of truncating them.

```cpp
pluss::series::print(std::cout, pluss::make_series({-1, 0, 1}).sinc()) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.8982e-17, 1.0000e+00, 3.8982e-17]
```

## Gotchas And Failure Modes

- This is the *normalized* sinc, `sin(pi x)/(pi x)`. The unnormalized
  `sin(x)/x` has its zeros at multiples of `pi`; if you want that, scale the
  argument by `1/pi` first.
- `sinc(n)` for integer `n` is not exactly `0` in floating point — it is a
  value near `4e-17`. Do not test it with `== 0.0`.
- The origin is handled: both the value (`1`) and the gradient (`0`) are the
  limit values, not `NaN`.
- Integral inputs promote to `float64`.
- `sinc_()` requires floating storage. It cannot preserve fractional results
  in an integral receiver; call `sinc()` when promotion is needed.

## Summary

`sinc()` is the normalized sinc, `sin(pi x)/(pi x)`:

- `sinc(0) = 1`, even, zeros at the non-zero integers
- smooth and finite everywhere
- differentiable everywhere; slope `0` at the origin, otherwise
  `(pi x cos(pi x) - sin(pi x)) / (pi x^2)`
- `sinc_()` is the in-place form

## See Also

- [`Series::sin`](sin.md): the numerator, without the normalization or the
  removable singularity.
- [`Series::i0`](i0.md): another smooth entire special function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sin.html">
    <span class="label">Previous</span>
    <span class="title">sin</span>
  </a>
  <a class="next-link" href="sinh.html">
    <span class="label">Next</span>
    <span class="title">sinh</span>
  </a>
</div>
