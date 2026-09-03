<!-- pluss-hand-authored -->

# Series::reshape

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::reshape(pluss::RangeView<std::int64_t> new_sizes) const -> Series
```

## What It Does

Changes the logical shape with a view-if-possible, contiguous-copy-if-needed
policy.

This is the forgiving shape API.

That wording matters because `reshape()` is often taught too loosely as "the
same as `view()`, but maybe slower." That is not the right mental model. The
right mental model is:

- `view()` protects aliasing
- `reshape()` protects the requested final logical result

So if your real contract is "I need these values under this final shape,"
`reshape()` is usually the correct default.

## Parameters

**new_sizes** (`pluss::RangeView<std::int64_t>`)

The target logical shape.

Common call forms:

- brace literals such as `{3, 2}`
- another series' [`sizes()`](sizes.md) result

The requested shape must preserve total `numel()`. `reshape()` is flexible
about layout, not flexible about how many values exist.

## Returns

`Series`
    A series with the requested logical shape.

That returned series may be:

- a true aliasing view, if the source is already view-compatible
- a new contiguous result, if the source must be normalized first

So the promise is about the final logical shape, not about which storage route
was taken to get there.

## Autograd Behavior

`reshape()` preserves reverse-mode connectivity in both execution paths.

There are really two forward stories:

1. direct view path
2. fallback contiguous-materialization path

The important guarantee is that both remain connected to the original source.
Even when `reshape()` has to normalize a non-contiguous input, gradients still
flow back to the original tensor rather than stopping at the staging copy.

## Shape Policy

The key distinction is between logical value order and physical
storage order.

When the input is already contiguous:

- `reshape()` behaves like `view()`

When the input is non-contiguous:

- `reshape()` preserves the current logical value order
- materializes that order contiguously
- then applies the requested final shape

That is why reshaping a transpose does not jump back to the pre-transpose
row-major interpretation.

## Verified Examples

Example 1. On a contiguous source, `reshape()` behaves like `view()`.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.reshape({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

So on already-compatible storage, `reshape()` does not invent extra work. It
simply takes the direct route.

Example 2. A non-contiguous transpose still reshapes successfully.

```cpp
std::cout << mat.transpose(0, 1).reshape({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

This is the example that teaches the method correctly. The result follows the
transposed logical order, not the original row-major order of `mat`.

Example 3. Borrowed shape metadata can be passed directly here too.

```cpp
auto shape_template = pluss::make_series({
    {10.0, 20.0},
    {30.0, 40.0},
    {50.0, 60.0},
});

std::cout << mat.reshape(shape_template.sizes());
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

Example 4. Gradients still flow through the non-contiguous fallback path.

```cpp
auto tracked_mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
tracked_mat.tracks_slope_(true);

tracked_mat.transpose(0, 1)
    .reshape({3, 2})
    .select(0, 1)
    .sum()
    .reverse();

std::cout << tracked_mat.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[0, 1, 0],
  [0, 1, 0]]
```

The selected row of the reshaped result corresponds to the original values `2`
and `5`, so those are exactly the source positions that receive gradient.

## Notes

- `reshape()` is usually the safer default when your contract is about the
  final logical shape.
- Equal `numel()` is still mandatory.
- Non-contiguous inputs can force materialization, but the method remains
  differentiable.
- If true aliasing is the requirement, use [`view()`](view.md) instead.

## See Also

- [`Series::view`](view.md): strict no-copy reinterpretation.
- [`Series::transpose`](transpose.md), [`Series::permute`](permute.md): common
  sources of non-contiguous layouts that later force `reshape()` onto its
  fallback path.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeat_interleave.html">
    <span class="label">Previous</span>
    <span class="title">repeat_interleave</span>
  </a>
  <a class="next-link" href="retain_grad.html">
    <span class="label">Next</span>
    <span class="title">retain_grad</span>
  </a>
</div>
