<!-- pluss-hand-authored -->

# Series::hypot

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::hypot(const Series& other) const -> Series
Series& Series::hypot_(const Series& other)
```

## What It Does

`hypot()` computes `sqrt(a^2 + b^2)` elementwise — the Euclidean length of the
right triangle with legs `a` (the receiver) and `b` (`other`).

The reason it is a primitive rather than a two-line expression is range. The
obvious spelling, `(a.square() + b.square()).sqrt()`, forms `a^2` as an
intermediate. If `a` is around `1e200`, then `a^2` is `1e400`, which overflows
`float64` to `inf`, and the final `sqrt` returns `inf` even though the true
answer is a perfectly ordinary `1e200`.

`hypot()` avoids that by factoring out the larger magnitude first:

```
hi = max(|a|, |b|)
lo = min(|a|, |b|)
result = hi * sqrt(1 + (lo / hi)^2)
```

Now the squared term is `(lo/hi)^2`, which is always in `[0, 1]`, so the
intermediate never overflows unless the true result itself is out of range.
When both inputs are `0` the result is `0` with no division by zero.

Shapes broadcast the same way as [`add()`](add.md). The result dtype is
floating point; an integer input is promoted.

## Parameters

**other** (`const Series&`)

The second leg `b`. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    The elementwise hypotenuse. `hypot_()` writes the result back into the
    receiver.

## Gradient

`hypot()` is differentiable. With `h` the output:

```
d/da hypot(a, b) = a / h
d/db hypot(a, b) = b / h
```

which are just the components of the unit vector pointing from the origin to
`(a, b)`. At `a = b = 0` the derivative is defined here as `0` rather than
`0/0`.

## Examples

Example 1. Classic Pythagorean triples.

```cpp
auto a = pluss::make_series({3.0, 5.0, 8.0});
auto b = pluss::make_series({4.0, 12.0, 15.0});

pluss::series::print(std::cout, a.hypot(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5.0000, 13.0000, 17.0000]
```

Example 2. The range-safety that motivates the method. The naive
`(a*a + b*b).sqrt()` would overflow here; `hypot()` does not.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({3e200}).hypot(pluss::make_series({4e200})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[5.0000e+200]
```

Example 3. Both legs zero gives zero, not `NaN`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0}).hypot(pluss::make_series({0.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.0000]
```

Example 4. `other` broadcasts. A scalar second leg is applied to every element.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({3.0, 5.0}).hypot(pluss::make_series(4.0)));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5.0000, 6.4031]
```

Example 5. The gradient is the unit direction vector.

```cpp
auto a = pluss::make_series({3.0, 4.0});
auto b = pluss::make_series({4.0, 3.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.hypot(b).sum().reverse();

std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
da=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 0.8]
db=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.6]
```

For legs `(3, 4)` the hypotenuse is `5`, and `(3/5, 4/5) = (0.6, 0.8)`.

Example 6. An integer input is promoted to a floating-point result.

```cpp
auto a = pluss::make_series({3, 4}, pluss::ScalarType::Int64);
auto b = pluss::make_series({4, 3}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, a.hypot(b));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5.0000, 5.0000]
```

Example 7. Non-broadcastable shapes fail.

```cpp
try {
  pluss::make_series({1.0, 2.0, 3.0}).hypot(pluss::make_series({1.0, 2.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- `hypot()` is the range-safe form on purpose. If you write
  `(a.square() + b.square()).sqrt()` yourself, you lose that safety.
- A genuinely infinite result (for example `hypot(inf, 1)`) is still `inf`;
  only the *spurious* overflow from the intermediate square is removed.
- The result is always floating point, even for integer inputs.
- `NaN` in either input propagates.

## Summary

`hypot(a, b)` is `sqrt(a^2 + b^2)` computed so the intermediate cannot spuriously
overflow:

- broadcasts like `add()`; always returns floating point
- `hypot(0, 0) = 0`
- differentiable, gradient `(a/h, b/h)` — the unit direction vector

## See Also

- [`Series::sqrt`](sqrt.md): the outer operation of the naive spelling
  `hypot()` replaces.
- [`Series::atan2`](atan2.md): the angle to go with this magnitude; together
  they are the polar form of `(b, a)`.
- [`Series::add`](add.md): the broadcasting rules `hypot()` follows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="histc.html">
    <span class="label">Previous</span>
    <span class="title">histc</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
