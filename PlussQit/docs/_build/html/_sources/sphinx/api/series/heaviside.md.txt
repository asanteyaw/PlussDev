<!-- pluss-hand-authored -->

# Series::heaviside

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::heaviside(const Series& values) const
Series Series::heaviside(double value) const
Series& Series::heaviside_(const Series& values)
Series& Series::heaviside_(double value)
Series& Series::heaviside_(const Series& values) const
Series& Series::heaviside_(double value) const
```

## A Step With An Explicit Boundary Value

For receiver element `x` and boundary value `v`, `heaviside()` returns `0`
when `x < 0`, `v` when `x == 0`, and `1` when `x > 0`. Passing `0`,
`0.5`, or `1` selects the common convention at the discontinuity. The
argument affects only exact zero, including `-0.0`.

The Series overload can vary the boundary value by position. Both operands must
have exactly the same dtype. They broadcast to a common shape, and the result
keeps that dtype. Complex inputs are rejected. The underscore forms preserve
the receiver's shape, dtype, and storage.

## Gradient

`heaviside()` is non-differentiable: it is constant away from zero and jumps
at zero. Use [`sigmoid()`](sigmoid.md) for a smooth trainable transition.

## In-Place Form

`heaviside_()` is the in-place sibling of [`heaviside()`](heaviside.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `heaviside_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0}, {3});
auto y = pluss::make_series({0.5, 0.5, 0.5}, {3});

x.heaviside_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.5, 1]
```

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0}, {3});
x.heaviside_(0.5);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.5, 1]
```

## Examples

### 1. Compare boundary conventions

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({-2.0, -0.0, 0.0, 3.0, nan});
pluss::series::print(std::cout, x.heaviside(0.0)) << '\n';
pluss::series::print(std::cout, x.heaviside(0.5)) << '\n';
pluss::series::print(std::cout, x.heaviside(1.0));
```

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0000, 0.0000, 0.0000, 1.0000, nan]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0000, 0.5000, 0.5000, 1.0000, nan]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0000, 1.0000, 1.0000, 1.0000, nan]
```

### 2. Broadcast boundary values by column

```cpp
auto x = pluss::make_series({{-2.0, 0.0, 3.0}, {0.0, -1.0, 2.0}});
auto at_zero = pluss::make_series({{0.25, 0.50, 0.75}});
pluss::series::print(std::cout, x.heaviside(at_zero));
```

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0.0000 0.5000 1.0000 ⎤
⎣ 0.2500 0.0000 1.0000 ⎦
```

The row broadcasts over both observations, but is read only at zero positions.

## Gotchas And Failure Modes

- A Series argument with a different dtype raises; convert it explicitly.
- Integer receivers cannot represent the `0.5` convention.
- `heaviside_()` rejects shape-expanding broadcasts.
- A `NaN` receiver propagates to the result.

## See Also

- [`Series::sign`](sign.md): summarize sign as `-1`, `0`, or `1`.
- [`Series::sigmoid`](sigmoid.md): build a smooth gate.
- [`Series::where`](where.md): select arbitrary values from a mask.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_sensitivity.html"><span class="label">Previous</span><span class="title">has_sensitivity</span></a>
  <a class="next-link" href="histc.html"><span class="label">Next</span><span class="title">histc</span></a>
</div>
