<!-- pluss-hand-authored -->

# Series::pow

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::pow(const Series& rhs) const
Series& Series::pow_(const Series& rhs) const
Series& Series::pow_(double rhs)
```

## What It Does

Returns an elementwise power.

`pow()` raises each element of `self` to the corresponding exponent in `rhs`
after broadcasting and dtype promotion have been resolved.

The public surface also accepts scalar exponent calls such as `x.pow(2.0)`,
which is usually the clearest way to express square, cube, square-root-like,
and other uniform exponent transforms.

## Parameters

**rhs** (`const Series&`)

The exponent side.

`rhs` may be:

- scalar-shaped
- the same shape as `self`
- broadcastable to the shape of `self`

## Returns

`Series`
    A new `Series` containing the elementwise result.

## Gradient

For positive-base differentiable floating inputs and scalar exponents, the
backward rule follows the familiar calculus identity:

`d/dx x^p = p * x^(p - 1)`

That is the main power-law gradient you should keep in mind when reasoning about or
debugging `pow()`.

## In-Place Form

`pow_()` is the in-place sibling of [`pow()`](pow.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `pow_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});

x.pow_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 9, 16]
```

```cpp
auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
x.pow_(2.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 9, 16]
```

## Examples

Example 1. A scalar exponent applies uniformly to every element.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.pow(2.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  1.0000  4.0000  9.0000 ⎤
⎣ 16.0000 25.0000 36.0000 ⎦
```

Example 2. A broadcasted exponent series can vary by column.

```cpp
auto pow_base = pluss::make_series({
    {1.0, 4.0, 9.0},
    {16.0, 25.0, 36.0},
});

std::cout << pow_base.pow(pluss::make_series({0.5, 1.0, 2.0}));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 81],
  [4, 25, 1296]]
```

Example 3. Fractional scalar exponents are a readable way to express root-like
transforms on positive values.

```cpp
std::cout << pluss::make_series({1.0, 4.0, 9.0}).pow(0.5);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 4. Negative bases with fractional exponents produce `nan` on the
verified floating route.

```cpp
std::cout << pluss::make_series({-1.0, 4.0}).pow(0.5);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[nan, 2]
```

Example 5. The scalar-exponent gradient follows the familiar power rule.

```cpp
auto pow_x = pluss::make_series({1.0, 2.0, 3.0});
pow_x.tracks_slope_(true);

auto pow_out = pow_x.pow(2.0);
pow_out.sum().reverse();

std::cout << "out=" << pow_out << "\n";
std::cout << "dx=" << pow_x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 4, 9]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 4, 6]
```

Example 6. Non-broadcastable exponent shapes fail.

```cpp
try {
  std::cout << base.pow(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- Fractional exponents and negative bases do not mix cleanly on the verified
  floating route; expect `nan`.
- Broadcasting rules still apply to the exponent side.
- `pow()` is more numerically delicate than add or multiply, so it deserves
  extra care around domain assumptions.

## Summary

Use `pow()` when you want exponent semantics directly, not when you merely want
repeated multiplication by a small fixed count. It is especially useful for
uniform power transforms, root-like transforms, and showing how domain issues
show up in floating arithmetic.

## See Also

- [`Series::mul`](mul.md): repeated multiplication without exponent semantics.
- [`Series::div`](div.md): inverse-side arithmetic.
- [`Series::add`](add.md): common companion when power transforms are part of a
  larger arithmetic expression.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pos.html">
    <span class="label">Previous</span>
    <span class="title">pos</span>
  </a>
  <a class="next-link" href="prelu.html">
    <span class="label">Next</span>
    <span class="title">prelu</span>
  </a>
</div>
