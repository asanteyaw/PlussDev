<!-- pluss-hand-authored -->

# Series::abs2

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::abs2() const
Series& Series::abs2_()
```

## What It Does

`abs2()` returns the squared magnitude of each value: `|x|^2`.

- for a real value that is just `x * x`, identical to [`square()`](square.md)
- for a complex value `z = a + b i` it is `a^2 + b^2` — a real, non-negative
  result, without the intermediate square root that `abs()` would take

The point of the method is that squared magnitude is what you usually want —
energy, power, a squared distance, the denominator of a normalization — and
computing it as `abs(x)` then squaring both does redundant work and, for
complex input, throws away the fact that the answer is real.

## Parameters

None.

## Returns

`Series` or `Series&`
    `|x|^2`, elementwise. Real for real input; real and non-negative for
    complex input. `abs2_()` overwrites the current storage.

## Gradient

`abs2()` is differentiable. For real input it is the [`square()`](square.md)
rule:

```
d/dx abs2(x) = 2 * x
```

## Examples

Example 1. On real input `abs2()` matches `square()`.

```cpp
pluss::series::print(std::cout, pluss::make_series({-2.0, 3.0}).abs2());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4.0000, 9.0000]
```

Example 2. The backward multiplier is `2 * x`.

```cpp
auto x = pluss::make_series({-2.0, 3.0, -4.0});
x.tracks_slope_(true);

x.abs2().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-4, 6, -8]
```

## Gotchas And Failure Modes

- For real input, `abs2()` and [`square()`](square.md) produce the same values;
  pick whichever reads better in context.
- Like `square()`, the result can overflow for large inputs, and an integer
  input keeps its dtype.
- `NaN` propagates.

## Summary

`abs2(x)` is the squared magnitude `|x|^2`:

- `x * x` for real input; `re^2 + im^2` for complex, giving a real result
- differentiable, backward multiplier `2 * x` on real input

## See Also

- [`Series::square`](square.md): identical for real input.
- [`Series::abs`](abs.md): the magnitude itself, `sqrt(abs2(x))`.
- [`Series::hypot`](hypot.md): `sqrt(a^2 + b^2)` from two separate real series.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="acos.html">
    <span class="label">Next</span>
    <span class="title">acos</span>
  </a>
</div>
