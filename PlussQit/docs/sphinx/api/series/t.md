<!-- pluss-hand-authored -->

# Series::t

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::t() const
Series& Series::t_() const
```

## What It Does

`t()` is the plain matrix transpose — a shorthand for `transpose(0, 1)`, but
only for 2-D series.

- on a **2-D** series it swaps the two axes, returning a view with the rows and
  columns exchanged
- on a series of rank `0`, `1`, or `3+` it is a **no-op**: the series is
  returned unchanged

That last rule is the thing to remember. `t()` will not complain about a 3-D
input — it simply hands it back as-is. If you want to swap two specific axes of
a higher-rank series, use [`swapaxes()`](swapaxes.md); to transpose the last
two axes of a batch of matrices, use [`mT()`](adjoint.md).

Like [`transpose()`](transpose.md), the result is a stride view, not a copy.

## Parameters

None.

## Returns

`Series`
    For a 2-D input, a transposed view (non-contiguous). For any other rank,
    the input unchanged.

## Gradient

`t()` is differentiable. It only relabels axes, so the backward transposes the
upstream gradient back — values pass through untouched.

## In-Place Form

`t_()` is the in-place sibling of [`t()`](t.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `t_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
x.t_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

## Examples

Example 1. Transposing a `2 x 3` matrix.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.t();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

The `strides=[1,3]` shows it is a view — no data moved.

Example 2. On a 1-D series, `t()` is a no-op.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).t();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 3. On a 3-D series, `t()` is also a no-op — it does **not** error and
does **not** reverse the axes.

```cpp
std::vector<double> data(12);
for (int i = 0; i < 12; ++i) data[i] = i + 1;
auto cube = pluss::make_series(data, {2, 2, 3});

auto result = cube.t();
std::cout << "shape=[";
for (auto x : result.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
shape=[2,2,3,]
```

Example 4. The backward transposes the gradient back.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
m.tracks_slope_(true);

m.t().sum().reverse();

pluss::series::print(std::cout, m.sensitivity());
```

### Output

```text
Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=
⎡ 1.0000 1.0000 1.0000 ⎤
⎣ 1.0000 1.0000 1.0000 ⎦
```

## Gotchas And Failure Modes

- `t()` only transposes 2-D. On any other rank it silently returns the input;
  it will not raise to warn you.
- For a higher-rank swap use [`swapaxes()`](swapaxes.md); for batched matrix
  transpose use [`mT()`](adjoint.md).
- The 2-D result is a non-contiguous view.

## Summary

`t(x)` is the 2-D matrix transpose:

- swaps the two axes of a 2-D series (a view); a no-op on any other rank
- differentiable, backward transposes the gradient back

## See Also

- [`Series::transpose`](transpose.md): swap any two named axes, any rank.
- [`Series::swapaxes`](swapaxes.md): the higher-rank two-axis swap.
- [`Series::mT`](adjoint.md): transpose the last two axes of a batch.
- [`Series::permute`](permute.md): reorder all axes at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="swapdims.html">
    <span class="label">Previous</span>
    <span class="title">swapdims</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
