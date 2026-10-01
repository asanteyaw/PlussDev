<!-- pluss-hand-authored -->

# Series::floor_divide

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::floor_divide(const Series& other) const
Series Series::floor_divide(double other) const
Series& Series::floor_divide_(const Series& other)
Series& Series::floor_divide_(double other)
Series& Series::floor_divide_(const Series& rhs) const
Series& Series::floor_divide_(double rhs) const
```

## What It Does

Divides elementwise and rounds the quotient **toward negative infinity**:

```
floor_divide(a, b) = floor(a / b)
```

This is true floor division, so the direction is determined by the number line,
not by the sign of the operands. `floor_divide(-7, 2)` is `-4`, not `-3`, while
`floor_divide(-8, -3)` is `2` because the quotient is positive.

It pairs with [`remainder()`](remainder.md). For every nonzero divisor, the two
operations preserve the Euclidean reconstruction:

```
a = floor_divide(a, b) * b + remainder(a, b)
```

The remainder follows the divisor's sign. Choosing `floor_divide()` rather than
truncating division is therefore not only a rounding decision; it determines
which remainder convention makes the reconstruction hold.

Dtype follows type promotion of the inputs: **two integral operands give an
integral result**; anything floating gives a floating result. For the scalar
overload, an integer-valued scalar against an integral series keeps the result
integral; a fractional scalar makes it floating.

## Parameters

**other** (`const Series&` or `double`): the divisor `b`. A `Series` broadcasts
against the receiver; a `double` applies the same divisor everywhere.

## Returns

`Series` or `Series&`
    `floor(a / b)`, integral when both operands are integral, otherwise
    floating. `floor_divide_()` writes the result back into the receiver.

## Gradient

`floor_divide()` is a rounding operation with a locally constant value, so it
is **not differentiable** — a reverse pass through it alone raises. For a
differentiable quotient use [`div()`](div.md).

## In-Place Form

`floor_divide_()` is the in-place sibling of [`floor_divide()`](floor_divide.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `floor_divide_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({7.0, -7.0, 8.0}, {3});
auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});

x.floor_divide_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, -4, 4]
```

```cpp
auto x = pluss::make_series({7.0, -7.0, 8.0}, {3});
x.floor_divide_(2.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, -4, 4]
```

## Examples

Example 1. All four sign combinations make the rounding direction visible.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({7, -7, 8, -8})
        .floor_divide(pluss::make_series({2, 2, -3, -3})));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[3, -4, -3, 2]
```

`-7 / 2 = -3.5` floors to `-4`. Likewise, `8 / -3` is approximately `-2.67`
and floors to `-3`; changing both signs makes `-8 / -3` positive, so it floors
to `2`.

Example 2. Floating operands give a floating result.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({7.5, -7.5}).floor_divide(pluss::make_series({2.0, 2.0})));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.0000, -4.0000]
```

Example 3. A fractional scalar promotes an integral receiver to floating point.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({7, 8}).floor_divide(2.5));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.0000, 3.0000]
```

The receiver is `int64`, but `2.5` cannot be represented as an integer divisor,
so the operation promotes before dividing. By contrast, `floor_divide(2.0)`
recognizes the scalar as integer-valued and keeps an integral result.

Example 4. The in-place form is appropriate only when the result fits the
receiver's existing shape and dtype.

```cpp
auto values = pluss::make_series({7, -7});
const void* storage = values.raw_data();

values.floor_divide_(2.0);
pluss::series::print(std::cout, values) << '\n';
std::cout << "same storage: " << std::boolalpha
          << (storage == values.raw_data());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, -4]
same storage: true
```

An integer-valued scalar leaves the result in `int64`, so this update is safe.
Replacing `2.0` with `2.5` would require a floating result and therefore cannot
be performed in place on this integer receiver; use `floor_divide(2.5)` instead.

## Gotchas And Failure Modes

- It floors, it does not truncate: a negative quotient rounds *away* from zero.
  For truncation toward zero, use `div()` then [`trunc()`](trunc.md).
- Two integer operands give an integer result; do not expect a float back from
  `int.floor_divide(int)`.
- A zero divisor has no valid integer floor quotient. Validate or mask divisors
  before calling when zero is possible; do not build an algorithm around a
  backend-specific `inf` or `NaN` result from floating division by zero.
- `floor_divide_()` preserves storage. It rejects a broadcast that changes the
  receiver shape and a result dtype that cannot be stored safely in the receiver.
- Not differentiable.

## Summary

`floor_divide(a, b)` is `floor(a / b)`:

- rounds toward negative infinity, including for negative results
- integral in, integral out; floating otherwise (scalar: integral only for an
  integer-valued scalar)
- pairs with [`remainder()`](remainder.md)
- not differentiable; `floor_divide_()` is the in-place form

## See Also

- [`Series::remainder`](remainder.md): the matching divisor-signed modulo.
- [`Series::div`](div.md): the un-rounded, differentiable quotient.
- [`Series::fmod`](fmod.md): the truncated-division remainder (pairs with
  truncation toward zero, not flooring).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="fmax.html">
    <span class="label">Next</span>
    <span class="title">fmax</span>
  </a>
</div>
