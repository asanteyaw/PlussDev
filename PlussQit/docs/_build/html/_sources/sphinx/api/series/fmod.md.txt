<!-- pluss-hand-authored -->

# Series::fmod

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::fmod(const Series& other) const -> Series
Series& Series::fmod_(const Series& other)
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

**other** (`const Series&`)

The divisor `b`. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    The truncated-division remainder, with the sign of `a`. Integer inputs stay
    integer. Division by `0` yields `NaN` (floating point). `fmod_()` writes the
    result back into the receiver.

## Gradient

`fmod()` is not differentiable. A `Series` produced by it does not carry
reverse tracking.

(Where a subgradient is wanted, `fmod(a, b)` changes one-for-one with `a`
between the jumps, and steps by `b` at each jump; it is not wired into
autograd.)

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

## Gotchas And Failure Modes

- `fmod()` follows the sign of `a`; [`remainder()`](remainder.md) follows the
  sign of `b`. They agree only when `a` and `b` already share a sign.
- Division by `0` gives `NaN` on floating-point inputs; it does not throw.
- Not differentiable.
- `NaN` in either input propagates.

## Summary

`fmod(a, b)` is the truncated-division remainder, signed like `a`:

- `a - trunc(a / b) * b`
- broadcasts like `add()`; preserves integer dtype
- `NaN` on divide-by-zero; not differentiable

## See Also

- [`Series::remainder`](remainder.md): the modulo whose result is signed like
  the divisor.
- [`Series::div`](div.md): the quotient this is the leftover of.
- [`Series::trunc`](trunc.md): the rounding rule `fmod()` uses internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="frac.html">
    <span class="label">Next</span>
    <span class="title">frac</span>
  </a>
</div>
