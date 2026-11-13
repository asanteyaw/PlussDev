<!-- pluss-hand-authored -->

# Series::sub

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sub(const Series& rhs) const
Series Series::sub(const Series& rhs, double alpha) const
Series& Series::sub_(const Series& rhs)
```

## What It Does

Returns an elementwise difference.

The plain overload computes:

`self - rhs`

The scaled overload computes:

`self - alpha * rhs`

That second form is important because it is often easier to reason about than
manually spelling the scale outside the method. It is also the subtraction-side
mirror of [`add(rhs, alpha)`](add.md).

The public surface also accepts ordinary scalar RHS calls such as `x.sub(2.0)`.
When you want a scaled scalar subtraction, the verified stable route is to pass
a scalar-shaped `Series` to the `rhs` overload.

## Overload Map

`sub(rhs)`

Plain elementwise subtraction after broadcasting.

`sub(rhs, alpha)`

Scaled subtraction. Use this when the right-hand side should be multiplied
before it is subtracted.

`sub(scalar)`

Public scalar convenience for the plain-subtract case.

## Parameters

**rhs** (`const Series&`)

The right-hand side.

`rhs` may match `self` exactly, be scalar-shaped, or be broadcastable to the
shape of `self`.

**alpha** (`double`)

A scale factor applied to `rhs` before subtraction.

This changes the meaning of the call materially:

- `x.sub(y)` means `x - y`
- `x.sub(y, 0.25)` means `x - 0.25 * y`
- `x.sub(y, -1.0)` is mathematically the same as `x + y`

## Returns

`Series`
    A new `Series` containing the elementwise result.

## Gradient

For differentiable dtypes:

- the gradient with respect to `self` is `1`
- the gradient with respect to `rhs` is `-alpha` on the scaled overload, or
  `-1` on the plain overload

This makes `sub()` a useful method for showing sign flow in reverse mode.

## In-Place Form

`sub_()` is the in-place sibling of [`sub()`](sub.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({10.0, 20.0, 30.0}, {3});
auto y = pluss::make_series({1.0, 2.0, 3.0}, {3});

auto& ref = x.sub_(y);
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9, 18, 27]
same object: true
```

## Examples

Example 1. A scalar RHS is subtracted from every element.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.sub(2.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1.0000 0.0000 1.0000 ⎤
⎣  2.0000 3.0000 4.0000 ⎦
```

Example 2. A scaled scalar subtraction uses a scalar-shaped `Series` rhs on the
 verified route.

```cpp
std::cout << pluss::series::print(
    base.sub(pluss::make_series(10.0), 0.5));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -4.0000 -3.0000 -2.0000 ⎤
⎣ -1.0000  0.0000  1.0000 ⎦
```

Example 3. A row-shaped RHS broadcasts across the leading dimension.

```cpp
auto row = pluss::make_series({10.0, 20.0, 30.0});
std::cout << base.sub(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-9, -18, -27],
  [-6, -15, -24]]
```

Example 4. `alpha` scales the broadcasted RHS before it is subtracted.

```cpp
std::cout << base.sub(row, 0.25);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.5, -3, -4.5],
  [1.5, 0, -1.5]]
```

Example 5. Reverse-mode sensitivities keep the expected signs.

```cpp
auto sub_x = pluss::make_series({1.0, 2.0, 3.0});
auto sub_y = pluss::make_series({10.0, 20.0, 30.0});
sub_x.tracks_slope_(true);
sub_y.tracks_slope_(true);

auto sub_out = sub_x.sub(sub_y, 0.5);
sub_out.sum().reverse();

std::cout << "out=" << sub_out << "\n";
std::cout << "dx=" << sub_x.sensitivity() << "\n";
std::cout << "dy=" << sub_y.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-4, -8, -12]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.5, -0.5, -0.5]
```

Example 6. Non-broadcastable shapes are rejected.

```cpp
try {
  std::cout << base.sub(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- `alpha` scales `rhs` before subtraction; it is not a post-hoc scale on the
  difference.
- The subtraction family is sign-sensitive in both forward and backward
  behavior.
- Scalar subtraction is verified directly. For scaled scalar subtraction, the
  stable public pattern is a scalar-shaped `Series` rhs.

## Summary

`sub()` is best understood as "subtract a broadcastable, optionally pre-scaled
RHS." Once that mental model is stable, the plain overload, the broadcast case,
and the gradient signs all become predictable.

## Aliases

**`subtract()`** / **`subtract_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `sub()` / `sub_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::add`](add.md): additive mirror with the same `alpha` idea.
- [`Series::mul`](mul.md): elementwise multiplication.
- [`Series::div`](div.md): elementwise division.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strides.html">
    <span class="label">Previous</span>
    <span class="title">strides</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
