<!-- pluss-hand-authored -->

# Series::frac

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::frac() const -> Series
Series& Series::frac_()
Series& Series::frac_() const
```

## What It Does

`frac()` returns the fractional part of each value, defined as

```
frac(x) = x - floor(x)
```

Because it subtracts `floor()` (which rounds toward negative infinity), the
result is always in `[0, 1)`, even for negative inputs:

- `frac(1.25) = 0.25`
- `frac(-1.25) = 0.75` — not `-0.25`
- `frac(3.0) = 0.0`

That is the "sawtooth" fractional part: it rises linearly from `0` to just
below `1` and drops back to `0` at every integer. It is the tool for wrapping a
continuous coordinate into a unit cell — texture coordinates, phase into one
period, a position into `[0, 1)`.

`frac()` requires a floating-point input.

## Parameters

None.

## Returns

`Series` or `Series&`
    `x - floor(x)`, always in `[0, 1)`. `frac_()` overwrites the current
    storage.

## Gradient

`frac()` is differentiable, with derivative `1` between the integer jumps:

```
d/dx frac(x) = 1
```

`floor(x)` is locally constant, so subtracting it does not change the slope.
The jump at each integer is a measure-zero set and is not represented in the
gradient.

## In-Place Form

`frac_()` is the in-place sibling of [`frac()`](frac.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `frac_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.75, -1.25});

auto& ref = x.frac_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.75, 0.75]
same object: true
```

## Examples

Example 1. The fractional part is always non-negative, including for negatives.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.25, -1.25, 3.0, -0.75}).frac());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.2500, 0.7500, 0.0000, 0.2500]
```

`-1.25 - floor(-1.25) = -1.25 - (-2) = 0.75`, and
`-0.75 - (-1) = 0.25`.

Example 2. The gradient is `1` everywhere the function is smooth.

```cpp
auto x = pluss::make_series({1.25, 2.75, 3.5});
x.tracks_slope_(true);

x.frac().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

Example 3. `frac()` is floating-point only, because it is built on
[`floor()`](floor.md).

```cpp
try {
  pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).frac();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
floor: dtype not implemented
```

## Gotchas And Failure Modes

- `frac()` uses `floor()`, so the result of a negative input is its distance
  *above* the next lower integer, not `x - trunc(x)`. `frac(-1.25)` is `0.75`,
  not `-0.25`.
- Integer input raises (the error names `floor`). Convert to a floating-point
  dtype first.
- `NaN` propagates.

## Summary

`frac(x)` is the sawtooth fractional part `x - floor(x)`:

- always in `[0, 1)`, including for negatives
- floating-point input only
- differentiable, derivative `1` between integer jumps

## See Also

- [`Series::floor`](floor.md): the piece subtracted off; `x = floor(x) +
  frac(x)`.
- [`Series::trunc`](trunc.md): rounds toward zero instead, giving a
  signed fractional part when subtracted.
- [`Series::remainder`](remainder.md): `frac(x)` is `remainder(x, 1.0)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmod.html">
    <span class="label">Previous</span>
    <span class="title">fmod</span>
  </a>
  <a class="next-link" href="frexp.html">
    <span class="label">Next</span>
    <span class="title">frexp</span>
  </a>
</div>
