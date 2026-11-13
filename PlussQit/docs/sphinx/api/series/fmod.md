<!-- pluss-hand-authored -->

# Series::fmod

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::fmod(const Series& other) const
Series Series::fmod(double other) const
Series& Series::fmod_(const Series& other)
Series& Series::fmod_(double other)
Series& Series::fmod_(const Series& rhs) const
Series& Series::fmod_(double rhs) const
```

## What It Does

`fmod()` is the elementwise remainder of `a / b` that **keeps the sign of the
dividend** `a`:

```
fmod(a, b) = a - trunc(a / b) * b
```

`trunc` rounds toward zero, so `fmod` matches the behavior of C's `fmod` and of
the `%` operator on signed integers in most languages:

- `fmod(-5, 3) = -2` — same sign as `-5`
- `fmod(5, -3) = 2` — same sign as `5`

This is the right choice when you want "the leftover after taking out whole
copies of `b`, counting toward zero." For a remainder that instead follows the
sign of the divisor (a true modulo), use [`remainder()`](remainder.md).

## Parameters

**other** (`const Series&` or `double`)

The divisor `b`. A `Series` broadcasts against the receiver; a `double` applies
the same divisor to every element.

## Returns

`Series` or `Series&`
    The truncated-division remainder, with the sign of `a`. Integer inputs stay
    integer. Division by `0` yields `NaN` (floating point). `fmod_()` writes the
    result back into the receiver.

## Gradient

`fmod()` is differentiable. `fmod(a, b) = a - trunc(a / b) * b`, and `trunc`
rounds to a locally constant integer, so between the jumps:

```
d/da fmod(a, b) = 1
d/db fmod(a, b) = -trunc(a / b)
```

The backward passes the upstream sensitivity straight through to the dividend
and routes `-trunc(a / b)` to the divisor. Gradient does not flow through the
discrete step where `trunc(a / b)` changes.

## In-Place Form

`fmod_()` is the in-place sibling of [`fmod()`](fmod.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `fmod_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
auto y = pluss::make_series({3.0, 3.0, 3.0}, {3});

x.fmod_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, -1]
```

```cpp
auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
x.fmod_(3.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, -1]
```

## Examples

Example 1. The sign of the result follows the dividend.

```cpp
auto a = pluss::make_series({-5.0, 5.0, -5.0, 5.5});
auto b = pluss::make_series({3.0, -3.0, -3.0, 2.0});

pluss::series::print(std::cout, a.fmod(b));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-2.0000, 2.0000, -2.0000, 1.5000]
```

`-5 = -1*3 + -2`, `5 = -1*(-3) + 2`, `-5 = 1*(-3) + -2`, `5.5 = 2*2 + 1.5`.

Example 2. The same call on integer inputs stays integer.

```cpp
auto a = pluss::make_series({7, -7, 7}, pluss::ScalarType::Int64);
auto b = pluss::make_series({3, 3, -3}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, a.fmod(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, -1, 1]
```

Example 3. Contrast with [`remainder()`](remainder.md), which follows the sign
of the divisor.

```cpp
auto a = pluss::make_series({-5.0, 5.0});
auto b = pluss::make_series({3.0, -3.0});

std::cout << "fmod      = " << a.fmod(b) << "\n";
std::cout << "remainder = " << a.remainder(b) << "\n";
```

### Output

```text
fmod      = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-2, 2]
remainder = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, -1]
```

Example 4. Division by zero is `NaN`, not an error.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({5.0}).fmod(pluss::make_series({0.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[nan]
```

Example 5. A scalar divisor applies the same `b` to every element.

```cpp
auto a = pluss::make_series({7.0, -7.0, 10.0});

pluss::series::print(std::cout, a.fmod(3.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, -1.0000, 1.0000]
```

Example 6. The backward passes gradient straight through to the dividend and
routes `-trunc(a / b)` to the divisor.

```cpp
auto a = pluss::make_series({-5.0, 5.5, 7.0});
auto b = pluss::make_series({3.0, 2.0, 4.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.fmod(b).sum().reverse();

std::cout << "da=" << a.sensitivity();
std::cout << "db=" << b.sensitivity();
```

### Output

```text
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, -2, -1]
```

Each `da` entry is `1`. Each `db` entry is `-trunc(a / b)`:
`-trunc(-5/3) = 1`, `-trunc(5.5/2) = -2`, `-trunc(7/4) = -1`.

## Gotchas And Failure Modes

- `fmod()` follows the sign of `a`; [`remainder()`](remainder.md) follows the
  sign of `b`. They agree only when `a` and `b` already share a sign.
- Division by `0` gives `NaN` on floating-point inputs; it does not throw.
- The gradient through the dividend is `1`; no gradient flows through the
  discrete step where `trunc(a / b)` changes.
- `NaN` in either input propagates.

## Summary

`fmod(a, b)` is the truncated-division remainder, signed like `a`:

- `a - trunc(a / b) * b`; `b` may be a `Series` or a scalar
- broadcasts like `add()`; preserves integer dtype
- `NaN` on divide-by-zero
- differentiable: `d/da = 1`, `d/db = -trunc(a / b)`, no flow through the jumps

## See Also

- [`Series::remainder`](remainder.md): the modulo whose result is signed like
  the divisor.
- [`Series::div`](div.md): the quotient this is the leftover of.
- [`Series::trunc`](trunc.md): the rounding rule `fmod()` uses internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmin.html">
    <span class="label">Previous</span>
    <span class="title">fmin</span>
  </a>
  <a class="next-link" href="frac.html">
    <span class="label">Next</span>
    <span class="title">frac</span>
  </a>
</div>
