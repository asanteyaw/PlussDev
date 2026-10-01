<!-- pluss-hand-authored -->

# Series::lgamma

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::lgamma() const -> Series
Series& Series::lgamma_()
Series& Series::lgamma_()
```

## What It Does

Computes the natural logarithm of the absolute value of the gamma function,
elementwise:

```
lgamma(x) = ln( |Gamma(x)| )
```

The gamma function `Gamma(x)` is the continuous extension of the factorial. For
a positive integer `n`:

```
Gamma(n) = (n - 1)!
```

and more generally `Gamma(x + 1) = x * Gamma(x)`. Its value grows
super-exponentially, so `Gamma(171)` already overflows `float64`. Working in
log space sidesteps that: `lgamma()` stays finite far past the point where the
gamma function itself would blow up, and products of gamma terms become sums of
`lgamma` terms.

That is the reason this method exists. Almost anywhere the gamma function shows
up in practice — combinatorial coefficients, the beta function, the
normalizing constants of continuous distributions — the quantity you actually
want is a ratio or product of gamma values, and the numerically safe way to
form it is as a sum and difference of `lgamma` values, exponentiating only at
the very end if at all.

Anchor facts to carry:

- `lgamma(1) = 0` and `lgamma(2) = 0`, because `0! = 1! = 1` and `ln(1) = 0`.
- `lgamma(n + 1) = ln(n!)`. So `exp(lgamma(n + 1))` recovers `n!`.
- `lgamma(0.5) = ln(sqrt(pi))`, roughly `0.5724`.
- `Gamma` has poles at `0` and every negative integer. `lgamma` returns `+inf`
  at those points.
- Between the negative-integer poles `Gamma(x)` is finite and nonzero, and
  because `lgamma` takes an absolute value, `lgamma(x)` is finite there too even
  though `Gamma(x)` is negative on some of those intervals.

`lgamma()` is an out-of-place unary transform: shape, dtype, and platform are
preserved. `lgamma_()` overwrites the current storage in place.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`, each element replaced by
    `ln(|Gamma(x)|)`. `lgamma_()` returns a reference to the mutated current
    series.

The result dtype matches the input floating-point dtype.

## Gradient

`lgamma()` is differentiable. Its derivative is the digamma function, written
`psi`:

```
d/dx lgamma(x) = psi(x)
```

`psi(x)` is itself `d/dx ln(Gamma(x))`, and there is no way around evaluating it
directly: like `lgamma` it is a genuine special function with no elementary
closed form. The backward pass evaluates it with a recurrence-plus-asymptotic
routine, so the sensitivity is accurate across the whole positive axis and for
negative non-integers.

Anchor values, useful for spot-checking a reverse pass:

- `psi(1) = -0.5772157` (the negative of the Euler-Mascheroni constant)
- `psi(2) = 0.4227843`
- `psi(x + 1) = psi(x) + 1/x`, which is why consecutive integer arguments differ
  by a simple reciprocal
- `psi(x)` grows like `ln(x)` for large `x`

Where `Gamma` has a pole, `psi` does too: the sensitivity at `x = 0` or a
negative integer is `NaN`. `psi` itself is not differentiable here, so a second
reverse pass through `lgamma()` is not supported.

## In-Place Form

`lgamma_()` is the in-place sibling of [`lgamma()`](lgamma.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 5.0});

auto& ref = x.lgamma_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 3.1780538]
same object: true
```

## Examples

Example 1. `lgamma` on the small positive integers is `ln` of a shifted
factorial.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).lgamma());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 0.0000, 0.6931, 1.7918, 3.1781, 4.7875]
```

Reading it off: `lgamma(3) = ln(2!) = ln 2 = 0.6931`, `lgamma(4) = ln(3!) =
ln 6 = 1.7918`, `lgamma(6) = ln(120) = 4.7875`. The first two entries are both
`0` because `0!` and `1!` are both `1`.

Example 2. Round-tripping the factorial. `exp(lgamma(n + 1))` returns `n!`.

```cpp
auto n = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});

pluss::series::print(std::cout, n.add(1.0).lgamma().exp());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 6.0000, 24.0000, 120.0000]
```

For small `n` you could compute the factorial directly. The point is that the
left-hand side stays finite for `n` in the hundreds, where `n!` itself is far
out of range.

Example 3. Half-integer arguments. `lgamma(0.5)` is `ln(sqrt(pi))`.

```cpp
pluss::series::print(std::cout, pluss::make_series({0.5, 1.5, 2.5}).lgamma());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5724, -0.1208, 0.2847]
```

`lgamma(1.5) = ln(0.5 * sqrt(pi))` is negative because its argument to `ln` is
below `1`.

Example 4. The poles. `Gamma` diverges at `0` and at the negative integers, so
`lgamma` reports `+inf` there.

```cpp
pluss::series::print(std::cout, pluss::make_series({0.0, -1.0, -2.0}).lgamma());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[inf, inf, inf]
```

Example 5. Between the poles, `lgamma` is finite for negative non-integers
because it takes an absolute value first.

```cpp
pluss::series::print(std::cout, pluss::make_series({-0.5, -1.5, -2.5}).lgamma());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.2655, 0.8600, -0.0562]
```

`Gamma(-0.5)` is `-2 * sqrt(pi)`, a negative number; `lgamma(-0.5)` is the log
of its magnitude, `1.2655`.

Example 6. Large arguments stay in range. `Gamma(1000)` would overflow many
times over; its log does not.

```cpp
std::cout << pluss::make_series({100.0, 1000.0}).lgamma();
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[359.13421, 5905.2204]
```

Example 7. The backward pass is `digamma(x)`. With the input tracked, a reverse
pass populates the sensitivity with `psi` evaluated at each element.

```cpp
auto x = pluss::make_series({2.0, 3.0, 4.0});
x.tracks_slope_(true);

auto y = x.lgamma();
std::cout << "y tracks slope: " << (y.tracks_slope() ? "true" : "false") << "\n";

y.sum().reverse();
std::cout << "dx=";
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
y tracks slope: true
dx=Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.4228, 0.9228, 1.2561]
```

Reading it off: `psi(2) = 0.4228`, and each following entry adds a reciprocal,
`psi(3) = psi(2) + 1/2 = 0.9228`, `psi(4) = psi(3) + 1/3 = 1.2561`.

Example 7b. At a pole the sensitivity is `NaN`, matching the `+inf` in the
forward value. A negative non-integer still has a finite gradient.

```cpp
auto x = pluss::make_series({-0.5, -1.5, 0.0});
x.tracks_slope_(true);

x.lgamma().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0365, 0.7032, nan]
```

Example 8. `lgamma()` is floating-point only. Integer and boolean inputs raise
rather than promote.

```cpp
try {
  pluss::series::print(std::cout, pluss::make_series({1, 2, 3}).lgamma());
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
lgamma: dtype not implemented (float-only)
```

Example 9. `lgamma_()` overwrites the current storage in place. Note the `+inf`
where the input hits a pole.

```cpp
auto a = pluss::make_series({0.0, 1.0, 2.0});
a.lgamma_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[inf, 0.0000, 0.0000]
```

## Gotchas And Failure Modes

- `lgamma()` requires a floating-point input. Integer and boolean series raise
  `lgamma: dtype not implemented (float-only)`; call [`to()`](to.md) first.
- The argument is the gamma argument, not a factorial argument. For `ln(n!)`
  pass `n + 1`.
- Inputs at `0` or at negative integers return `+inf`, not `NaN`. `NaN` inputs
  propagate as `NaN`.
- The gradient (`digamma`) is `NaN` at those same poles. If a tracked `lgamma()`
  can see a non-positive integer, expect `NaN` in the sensitivity.
- `digamma` is not itself differentiable here, so a second reverse pass through
  `lgamma()` is unsupported.
- `lgamma()` returns the log of an absolute value. It tells you the magnitude of
  `Gamma(x)`, never its sign. On the intervals where `Gamma(x)` is negative you
  have lost that information.

## Summary

`lgamma()` is `ln(|Gamma(x)|)` as a shape-preserving unary transform:

- the log-space form of the factorial, finite where `Gamma` overflows
- `lgamma(n + 1) = ln(n!)`, so `exp(lgamma(n + 1))` recovers `n!`
- `+inf` at `0` and the negative integers; finite elsewhere
- differentiable, with backward `digamma(x)`; the gradient is `NaN` at the poles
- floating-point only; convert integral inputs with [`to()`](to.md)
- `lgamma_()` is the in-place form

## See Also

- [`Series::exp`](exp.md): the inverse step for turning an `lgamma` result back
  into a gamma or factorial value.
- [`Series::log`](log.md): plain elementwise natural log, for values that are
  not gamma arguments.
- [`Series::factorial`](factorial.md): the exact integer factorial, for the
  discrete counterpart of what `lgamma` extends continuously.
- [`Series::erf`](erf.md): another differentiable special function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lerp.html">
    <span class="label">Previous</span>
    <span class="title">lerp</span>
  </a>
  <a class="next-link" href="linearalg.html">
    <span class="label">Next</span>
    <span class="title">linearalg</span>
  </a>
</div>
