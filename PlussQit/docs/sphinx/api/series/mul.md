<!-- pluss-hand-authored -->

# Series::mul

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::mul(const Series& rhs) const -> Series
```

## What It Does

Returns an elementwise product.

`mul()` is the clean multiplicative counterpart to [`add()`](add.md) and
[`sub()`](sub.md). It multiplies corresponding values after broadcasting and
dtype promotion have been resolved.

The public surface also accepts ordinary scalar RHS calls such as `x.mul(2.0)`,
which is often the most readable way to scale an entire series.

## Parameters

**rhs** (`const Series&`)

The right-hand side.

`rhs` may be:

- the same shape as `self`
- scalar-shaped
- broadcastable to `self`

## Returns

`Series`
    A new `Series` containing the elementwise result.

## Gradient

For differentiable dtypes:

- the gradient with respect to `self` is `rhs`
- the gradient with respect to `rhs` is `self`

That simple exchange makes `mul()` one of the easiest elementwise nonlinear
surfaces to reason about in reverse mode.

## Examples

Example 1. A scalar RHS scales every element.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << base.mul(2.0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 4, 6],
  [8, 10, 12]]
```

Example 2. A 1D RHS can broadcast across the leading dimension.

```cpp
auto row = pluss::make_series({10.0, 20.0, 30.0});
std::cout << base.mul(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 40, 90],
  [40, 100, 180]]
```

Example 3. Non-contiguous inputs are fine; `mul()` materializes an ordinary
result series.

```cpp
auto transposed = base.transpose(0, 1);
pluss::series::print(std::cout,
    transposed.mul(pluss::make_series({10.0, 100.0})));
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 10.0000 400.0000 ⎤
⎢ 20.0000 500.0000 ⎥
⎣ 30.0000 600.0000 ⎦
```

Example 4. Multiplication also follows type promotion rules.

```cpp
auto mul_ints = pluss::series::arange(1, 5, 1, pluss::ScalarType::Int32);
auto mul_floats = pluss::series::full({4}, 0.5, pluss::ScalarType::Float32);
std::cout << mul_ints.mul(mul_floats);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.5, 1, 1.5, 2]
```

Example 5. In reverse mode, each side receives the other side's values.

```cpp
auto mul_x = pluss::make_series({1.0, 2.0, 3.0});
auto mul_y = pluss::make_series({10.0, 20.0, 30.0});
mul_x.tracks_slope_(true);
mul_y.tracks_slope_(true);

auto mul_out = mul_x.mul(mul_y);
mul_out.sum().reverse();

std::cout << "out=" << mul_out << "\n";
std::cout << "dx=" << mul_x.sensitivity() << "\n";
std::cout << "dy=" << mul_y.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 40, 90]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 20, 30]
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 6. Non-broadcastable shapes fail.

```cpp
try {
  std::cout << base.mul(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- Broadcasting can make `mul()` feel effortless, but incompatible shapes still
  fail immediately.
- The output may be promoted to a wider floating dtype when one side requires
  it.
- A non-contiguous input does not imply a non-contiguous output. `mul()`
  returns a fresh result series.

## Summary

Think of `mul()` as the simplest broadcast-aware scale-and-combine primitive on
`Series`. It is ideal for explicit feature scaling, elementwise gating, and
many reverse-mode sanity checks because the backward rule is easy to verify by
eye.

## See Also

- [`Series::add`](add.md): additive counterpart.
- [`Series::div`](div.md): multiplicative inverse counterpart.
- [`Series::pow`](pow.md): repeated multiplication with exponent semantics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movedim.html">
    <span class="label">Previous</span>
    <span class="title">movedim</span>
  </a>
  <a class="next-link" href="multinomial.html">
    <span class="label">Next</span>
    <span class="title">multinomial</span>
  </a>
</div>
