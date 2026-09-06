<!-- pluss-hand-authored -->

# Series::add

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::add(const Series& rhs) const -> Series
Series::add(const Series& rhs, double alpha) const -> Series
```

## What It Does

Returns an elementwise sum.

At its simplest, `add()` does what you expect: it adds `self` and `rhs`
position by position after broadcasting and dtype promotion have been resolved.
The richer part of the surface is the second overload:

`self.add(rhs, alpha)` computes

`self + alpha * rhs`

That means `alpha` does not scale the final result. It scales the right-hand
side before the addition happens.

The public surface also accepts ordinary scalar RHS calls such as
`x.add(2.0)`. When you need `alpha` scaling with a scalar value, the verified
stable pattern is to pass a scalar-shaped `Series` on the `rhs` overload.

## Overload Map

`add(rhs)`

Plain elementwise addition after broadcasting.

`add(rhs, alpha)`

Scaled addition. Use this when the right-hand side should be multiplied before
it is accumulated into `self`.

`add(scalar)`

Public scalar convenience for the plain-add case.

## Parameters

**rhs** (`const Series&`)

The right-hand side.

`rhs` may be:

- the same shape as `self`
- broadcastable to the shape of `self`
- scalar-shaped

Broadcasting follows the same usual trailing-dimension rules used across the
rest of `Series`.

**alpha** (`double`)

A scale factor applied to `rhs` before the addition.

This toggle matters because it changes the actual expression:

- `x.add(y)` means `x + y`
- `x.add(y, 0.25)` means `x + 0.25 * y`
- `x.add(y, -1.0)` is a readable way to express subtraction through the add
  family

## Returns

`Series`
    A new `Series` containing the elementwise result.

## Gradient

For differentiable dtypes, `add()` is one of the cleanest gradient surfaces:

- the gradient with respect to `self` is `1`
- the gradient with respect to `rhs` is `alpha` on the scaled overload, or `1`
  on the plain overload

That makes `add()` a good method to show the difference between value flow and
gradient scaling: `alpha` changes both the forward value and the backward
multiplier for `rhs`.

## Examples

Example 1. A scalar RHS is added to every element.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.add(2.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 3.0000 4.0000 5.0000 ⎤
⎣ 6.0000 7.0000 8.0000 ⎦
```

Example 2. When you want scalar-plus-`alpha`, the verified public route is a
scalar-shaped `Series` on the `rhs` overload.

```cpp
std::cout << pluss::series::print(
    base.add(pluss::make_series(10.0), 0.5));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 6.0000  7.0000  8.0000 ⎤
⎣ 9.0000 10.0000 11.0000 ⎦
```

That is `base + 0.5 * 10`, not `(base + 10) * 0.5`.

Example 3. A row-shaped `rhs` broadcasts across the leading dimension.

```cpp
auto row = pluss::make_series({10.0, 20.0, 30.0});
std::cout << base.add(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[11, 22, 33],
  [14, 25, 36]]
```

Example 4. `alpha` works with broadcasted `rhs` too.

```cpp
std::cout << base.add(row, 0.25);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3.5, 7, 10.5],
  [6.5, 10, 13.5]]
```

Example 5. Addition promotes dtype when needed.

```cpp
auto ints = pluss::series::arange(0, 4, 1, pluss::ScalarType::Int32);
auto floats = pluss::series::full({4}, 1.5, pluss::ScalarType::Float32);
std::cout << ints.add(floats);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.5, 2.5, 3.5, 4.5]
```

Example 6. The scaled overload also scales the backward sensitivity for `rhs`.

```cpp
auto add_x = pluss::make_series({1.0, 2.0, 3.0});
auto add_y = pluss::make_series({10.0, 20.0, 30.0});
add_x.tracks_slope_(true);
add_y.tracks_slope_(true);

auto add_out = add_x.add(add_y, 0.5);
add_out.sum().reverse();

std::cout << "out=" << add_out << "\n";
std::cout << "dx=" << add_x.sensitivity() << "\n";
std::cout << "dy=" << add_y.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6, 12, 18]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5, 0.5, 0.5]
```

Example 7. Non-broadcastable shapes fail loudly.

```cpp
try {
  std::cout << base.add(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- `alpha` scales `rhs`, not the final sum.
- Broadcasting is convenient, but it still follows strict shape rules.
- Scalar addition is public and verified. For scalar-plus-`alpha`, the reliable
  route is a scalar-shaped `Series` on the `rhs` overload.
- Dtype promotion can change the output dtype even when one side started as an
  integer series.

## Summary

Use `add()` for three closely related jobs: plain elementwise addition,
broadcasted addition, and scaled accumulation through `alpha`. The core mental
model is always `self + scaled_rhs`, where the scale is either `1` or the
explicit `alpha` you supply.

## See Also

- [`Series::sub`](sub.md): subtraction family.
- [`Series::mul`](mul.md): elementwise multiplication.
- [`Series::sum`](sum.md): reduction after arithmetic composition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="acosh.html">
    <span class="label">Previous</span>
    <span class="title">acosh</span>
  </a>
  <a class="next-link" href="addcdiv.html">
    <span class="label">Next</span>
    <span class="title">addcdiv</span>
  </a>
</div>
