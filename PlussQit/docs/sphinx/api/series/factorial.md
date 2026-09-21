<!-- pluss-hand-authored -->

# Series::factorial

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::factorial() const -> Series
```

## What It Does

`factorial()` returns `x!` for each element, interpreting every value as a
non-negative integer.

`x!` is the product of the integers from `1` to `x`, with the empty product
`0! = 1`:

- `0` maps to `1`
- `1` maps to `1`
- `4` maps to `24`
- `6` maps to `720`

This is the discrete, exact counterpart of the gamma function. Where
[`lgamma()`](lgamma.md) extends the factorial to a smooth curve over the whole
real line and works in log space, `factorial()` stays on the integers and
returns the exact product as an `int64`.

The exact-integer choice is what fixes the domain. `20!` is
`2432902008176640000`, the largest factorial that fits in a signed 64-bit
integer. `21!` does not fit, so the supported input range is `0` through `20`
inclusive; anything larger raises rather than silently overflowing.

Internally the result is a gather from the 21-entry table of factorials, so the
per-element step is a lookup rather than a running multiplication.

## Input Rules

`factorial()` accepts integral and boolean dtypes, matching the rest of the
small number-theory cluster ([`factor`](factor.md), [`prime`](prime.md),
[`is_prime`](is_prime.md)). The output dtype is always `int64`.

- every element must be at least `0`
- every element must be at most `20`
- boolean input is read numerically: `true` is `1` (so `1! = 1`), `false` is
  `0` (so `0! = 1`)
- a floating-point input is rejected, not rounded

## Returns

`Series`
    An `int64` `Series` with the same shape as `self`, where each element has
    been replaced by its factorial.

## Gradient

`factorial()` is not differentiable.

It is a discrete integer-to-integer lookup, and its result dtype is always
`int64`, so a `Series` produced by `factorial()` does not carry reverse
tracking. For a differentiable stand-in over a continuous argument, use
`exp(lgamma(x + 1))`.

## Examples

Example 1. The first eleven factorials.

```cpp
auto values = pluss::make_series(
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, values.factorial());
```

### Output

```text
Series(shape=[11], strides=[1], dtype=int64, platform=cpu, numel=11, contiguous=true)
data=[1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800]
```

The first two entries are both `1` because `0!` and `1!` are both the empty or
single-term product.

Example 2. Shape is preserved exactly, including rank. A scalar (rank-0)
`Series` returns a scalar; a one-element vector returns a one-element vector.

```cpp
auto scalar = pluss::make_series(6);
std::cout << scalar.factorial() << "\n";

auto one = pluss::make_series({6}, pluss::ScalarType::Int64);
std::cout << one.factorial();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=720
pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[720]
```

Example 3. Factorials are computed positionwise on any shape.

```cpp
std::cout << pluss::make_series({
    {0, 1, 2},
    {3, 4, 5},
}, pluss::ScalarType::Int64).factorial();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 2],
  [6, 24, 120]]
```

Example 4. A non-contiguous view is accepted; the result is a fresh contiguous
`int64` series in the view's visible order.

```cpp
auto grid = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

std::cout << grid.transpose(0, 1).factorial();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 24],
  [2, 120],
  [6, 720]]
```

Example 5. Boolean input is interpreted numerically, so every element is `0` or
`1` and every factorial is `1`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({true, false, true}).factorial());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

Example 6. Any integral input dtype is promoted to an `int64` result.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({3, 5, 7}, pluss::ScalarType::Int32).factorial());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[6, 120, 5040]
```

Example 7. `20` is the largest supported input. `20!` is exactly the largest
factorial representable as a signed 64-bit integer.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({20}, pluss::ScalarType::Int64).factorial());
```

### Output

```text
Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[2432902008176640000]
```

Example 8. `21` would overflow, so it is rejected rather than wrapping.

```cpp
try {
  pluss::make_series({21}, pluss::ScalarType::Int64).factorial();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
factorial: input too large; 21! overflows int64 (max supported input is 20)
```

Example 9. Negative inputs are rejected. The factorial is not defined for them.

```cpp
try {
  pluss::make_series({-1}, pluss::ScalarType::Int64).factorial();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
factorial: expects non-negative integer input
```

Example 10. A floating-point input is rejected up front rather than being
rounded to an integer.

```cpp
try {
  pluss::make_series({1.0, 2.0}).factorial();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
factorial: expects integral or bool dtype, got float64
```

Even a value like `6.0` that is an exact integer still raises: the check is on
the dtype, not the value. Convert first if the source really is a float column,
remembering that `to(Int64)` truncates toward zero:

```cpp
auto f = pluss::make_series({0.0, 1.0, 5.0});
pluss::series::print(std::cout, f.to(pluss::ScalarType::Int64).factorial());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 120]
```

Example 11. `factorial()` does not participate in autodiff.

```cpp
auto x = pluss::make_series({3, 4, 5}, pluss::ScalarType::Int64);
x.tracks_slope_(true);

auto y = x.factorial();
std::cout << "y tracks slope: " << (y.tracks_slope() ? "true" : "false");
```

### Output

```text
y tracks slope: false
```

## Gotchas And Failure Modes

- The supported input range is `0` through `20`. `21` and above raise
  `factorial: input too large`; there is no saturating or wrapping mode.
- Negative inputs raise `factorial: expects non-negative integer input`.
- Floating-point inputs raise
  `factorial: expects integral or bool dtype`. Round or truncate to an integer
  dtype yourself if that is what you intend.
- The result is always `int64`, regardless of the input integer width.
- Not differentiable. Use `exp(lgamma(x + 1))` where a smooth, tracked factorial
  is needed.

## Summary

`factorial()` is the exact integer factorial as a shape-preserving lookup:

- elementwise `x!` for integer `x` in `[0, 20]`, always returning `int64`
- `0! = 1`; boolean input collapses to `1`
- inputs above `20`, negative inputs, and floating-point inputs all raise
- not differentiable; the continuous analogue is [`lgamma()`](lgamma.md)

## See Also

- [`Series::lgamma`](lgamma.md): the smooth, differentiable extension;
  `exp(lgamma(x + 1)) == x!` for non-negative integer `x`.
- [`Series::factor`](factor.md): least prime factor, another discrete
  integer-analysis method in the same family.
- [`Series::prime`](prime.md): the nth prime by ordinal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factor.html">
    <span class="label">Previous</span>
    <span class="title">factor</span>
  </a>
  <a class="next-link" href="flatten.html">
    <span class="label">Next</span>
    <span class="title">flatten</span>
  </a>
</div>
