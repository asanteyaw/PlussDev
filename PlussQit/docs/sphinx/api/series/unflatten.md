<!-- pluss-hand-authored -->

# Series::unflatten

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::unflatten(
    std::int64_t dim,
    pluss::msharp::RangeView::Int sizes) const
```

## Expanding One Axis Into Several Axes

`unflatten()` replaces one receiver axis with a sequence of axes whose product
equals the original extent. Values retain their row-major order; only their
coordinates change. It is the precise inverse of flattening the same adjacent
axes when no intervening permutation has changed their order.

If the receiver has shape `[2,12]`, then `unflatten(1,{3,4})` produces
`[2,3,4]`. All axes before `dim` remain before the inserted axes, and all axes
after `dim` remain after them. Negative `dim` values are normalized from the
back, so `-1` selects the final axis.

`sizes` must be nonempty. Its extents must be non-negative, except that exactly
one `-1` may request inference from the selected receiver extent and the other
specified extents. The inferred extent must be integral and unambiguous. In
particular, `{2,-1}` can split an empty extent into `{2,0}`, whereas `{0,-1}`
is rejected because every inferred value would have product zero. Product
overflow is rejected before shape construction.

## Storage, Metadata, And Reverse Mode

The output preserves dtype and platform. When the receiver's storage layout
can represent the expanded axes directly, `unflatten()` returns a metadata
view rather than copying values; otherwise the reshape machinery materializes
the required layout. The receiver is never mutated.

Reverse mode collapses the inserted axes back into the original extent. Every
output coordinate maps to exactly one input coordinate, so no sensitivity is
summed merely because the rank changed.

## Examples

### 1. Split an axis with explicit extents

```cpp
const auto flat = pluss::series::arange(
    0.0, 24.0, 1.0, pluss::ScalarType::Float64).reshape({2, 12});
pluss::series::print(std::cout, flat.unflatten(1, {3, 4}));
```

#### Output

```text
Series(shape=[2,3,4], strides=[12,4,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data[0, :, :] =
⎡ 0.0000 1.0000  2.0000  3.0000 ⎤
⎢ 4.0000 5.0000  6.0000  7.0000 ⎥
⎣ 8.0000 9.0000 10.0000 11.0000 ⎦

data[1, :, :] =
⎡ 12.0000 13.0000 14.0000 15.0000 ⎤
⎢ 16.0000 17.0000 18.0000 19.0000 ⎥
⎣ 20.0000 21.0000 22.0000 23.0000 ⎦
```

The original final coordinate `j` becomes coordinates
`{j / 4, j % 4}`.

### 2. Infer one extent and use a negative axis

```cpp
const auto expanded = flat.unflatten(-1, {2, -1});
std::cout << "shape=" << expanded.sizes() << '\n';
```

#### Output

```text
shape=[2, 2, 6]
```

The selected extent is twelve. After fixing the first inserted extent at two,
the inferred extent is six.

### 3. Infer a zero extent without ambiguity

```cpp
const auto empty =
    pluss::series::empty({0}, pluss::ScalarType::Float64)
        .unflatten(0, {2, -1});
std::cout << "shape=" << empty.sizes() << '\n';
```

#### Output

```text
shape=[2, 0]
```

The known nonzero factor makes the inferred zero unique. By contrast,
`{0,-1}` is ambiguous and throws.

### 4. Differentiate through the shape view

```cpp
auto tracked = flat.clone();
tracked.tracks_slope_(true);
tracked.unflatten(1, {3, 4}).sum().reverse();
pluss::series::print(std::cout, tracked.sensitivity());
```

#### Output

```text
Series(shape=[2,12], strides=[12,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data=
⎡ 1.0000 1.0000 1.0000 1.0000 1.0000 ... 1.0000 ⎤
⎣ 1.0000 1.0000 1.0000 1.0000 1.0000 ... 1.0000 ⎦
```

### 5. Reject a non-factorable request

```cpp
try {
  (void)flat.unflatten(1, {5, -1});
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
unflatten: sizes do not multiply to the selected dimension
```

Multiple inferred extents, an empty `sizes` list, dimensions below `-1`, an
out-of-range axis, and overflowing products are also rejected.

## See Also

- [`flatten()`](flatten.md) combines adjacent axes.
- [`reshape()`](reshape.md) specifies the complete destination shape.
- [`view()`](view.md) requests a compatible metadata view.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unbind.html">
    <span class="label">Previous</span>
    <span class="title">unbind</span>
  </a>
  <a class="next-link" href="unfold.html">
    <span class="label">Next</span>
    <span class="title">unfold</span>
  </a>
</div>
