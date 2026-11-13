<!-- pluss-hand-authored -->

# Series::moveaxis

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::moveaxis(std::int64_t source, std::int64_t destination) const
```

[`movedim()`](movedim.md) is an exact alias.

## What It Does

`moveaxis()` takes the axis at position `source` and reinserts it at position
`destination`, sliding every other axis along to fill the gap. The relative
order of the axes that are *not* moved is preserved.

This is **not** a swap. Swapping two axes (`swapaxes`, `transpose`, `t`) only
exchanges the two named ones and leaves the rest alone. `moveaxis` shifts a
whole run of axes over. On a rank-3 series `moveaxis(0, 2)` gives axis order
`(1, 2, 0)`, whereas `swapaxes(0, 2)` gives `(2, 1, 0)`.

Both `source` and `destination` may be negative, counting from the end. The
result is a stride view — no data is copied.

## Parameters

**source** (`std::int64_t`)

The current position of the axis to move. Negative counts from the end.

**destination** (`std::int64_t`)

The position the axis should occupy in the result. Negative counts from the end.

## Returns

`Series`
    A view with the axes reordered. Same dtype and platform. Non-contiguous in
    general.

## Gradient

`moveaxis()` is differentiable. It only relabels axes, so the backward moves the
axis of the upstream gradient back from `destination` to `source`. Values are
untouched.

## Examples

Example 1. Moving axis `0` to the end of a `2 x 3 x 4` series.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto r = t3.moveaxis(0, 2);
std::cout << "shape=[";
for (auto x : r.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
shape=[3,4,2,]
```

Axis `0` (size `2`) went to the back; axes `1` and `2` (sizes `3`, `4`) slid
forward, keeping their order.

Example 2. Contrast with [`swapaxes()`](swapaxes.md), which only exchanges the
two named axes.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto a = t3.moveaxis(0, 2);
auto b = t3.swapaxes(0, 2);
std::cout << "moveaxis(0,2)=[";
for (auto x : a.sizes()) std::cout << x << ",";
std::cout << "]  swapaxes(0,2)=[";
for (auto x : b.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
moveaxis(0,2)=[3,4,2,]  swapaxes(0,2)=[4,3,2,]
```

`moveaxis` keeps the `3, 4` order; `swapaxes` reverses to `4, 3`.

Example 3. The values follow the axes. Here a `2 x 2 x 3` cube with
`moveaxis(0, 2)`.

```cpp
std::vector<double> d(12);
for (int i = 0; i < 12; ++i) d[i] = i + 1;
auto cube = pluss::make_series(d, {2, 2, 3});

pluss::series::print(std::cout, cube.moveaxis(0, 2));
```

### Output

```text
Series(shape=[2,3,2], strides=[3,1,6], dtype=float64, platform=cpu, numel=12, contiguous=false)
data[0, :, :] =
⎡ 1.0000 7.0000 ⎤
⎢ 2.0000 8.0000 ⎥
⎣ 3.0000 9.0000 ⎦

data[1, :, :] =
⎡ 4.0000 10.0000 ⎤
⎢ 5.0000 11.0000 ⎥
⎣ 6.0000 12.0000 ⎦
```

Example 4. Moving an axis to its own position is a no-op — and stays
contiguous.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto r = t3.moveaxis(1, 1);
std::cout << "shape=[";
for (auto x : r.sizes()) std::cout << x << ",";
std::cout << "] contiguous=" << (r.is_contiguous() ? "true" : "false");
```

### Output

```text
shape=[2,3,4,] contiguous=true
```

Example 5. Differentiable: the gradient flows back to the original layout.

```cpp
std::vector<double> d(12);
for (int i = 0; i < 12; ++i) d[i] = i + 1;
auto cube = pluss::make_series(d, {2, 2, 3});
cube.tracks_slope_(true);

cube.moveaxis(0, 2).sum().reverse();

std::cout << "grad shape=[";
for (auto z : cube.sensitivity().sizes()) std::cout << z << ",";
std::cout << "]";
```

### Output

```text
grad shape=[2,2,3,]
```

## Gotchas And Failure Modes

- It moves one axis and shifts the rest — it is not a two-axis swap. Reach for
  [`swapaxes()`](swapaxes.md) when you want a swap.
- `source` and `destination` out of range raise.
- The result is a non-contiguous view unless `source == destination`.
- To move several axes at once, use [`permute()`](permute.md) with the full
  target order.

## Summary

`moveaxis(source, destination)` relocates one axis and slides the others to
fill in:

- preserves the relative order of the axes that do not move
- distinct from a swap: rank-3 `moveaxis(0, 2)` is order `(1, 2, 0)`, not
  `(2, 1, 0)`
- a zero-copy view; differentiable

## See Also

- [`Series::movedim`](movedim.md): exact alias of this method.
- [`Series::swapaxes`](swapaxes.md): exchange two axes instead of moving one.
- [`Series::permute`](permute.md): give the full axis order directly.
- [`Series::transpose`](transpose.md): swap two named axes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mode.html">
    <span class="label">Previous</span>
    <span class="title">mode</span>
  </a>
  <a class="next-link" href="movedim.html">
    <span class="label">Next</span>
    <span class="title">movedim</span>
  </a>
</div>
