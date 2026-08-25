<!-- pluss-hand-authored -->

# Series::sizes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sizes() const -> pluss::RangeView<std::int64_t>
```

## What It Does

Returns the full logical shape of the current `Series` as a borrowed
range-like view.

This is the whole-shape counterpart to [`size()`](size.md):

- `size(dim)` asks for one axis
- `sizes()` asks for the entire rank in order

That sounds simple, but it is one of the load-bearing metadata APIs in the
entire `Series` surface because it is both:

- an inspection tool
- a wiring tool for other shape-taking methods

In practice, `sizes()` is how you answer questions like:

- "what rank and extents does this handle currently expose?"
- "what shape did that transpose or unsqueeze produce?"
- "can I feed this shape directly into `view()` or `reshape()`?"

## Parameters

This method takes no parameters.

There are no toggles because `sizes()` is pure metadata inspection. If you want
per-axis choice, you are already in [`size()`](size.md) territory instead.

## Returns

`pluss::RangeView<std::int64_t>`
    A lightweight borrowed view of the current axis extents.

The teaching point here is not the exact type spelling. The teaching point is:

- cheap to inspect
- cheap to pass onward
- not an owning snapshot

## Borrowed Lifetime

Treat the returned object as metadata tied to the `Series` it came from.

That usually means it is excellent for:

- immediate printing
- immediate looping
- immediate argument passing into another API

It is not meant to behave like an independent shape vector that you store long
after the originating `Series` changes or disappears.

## Verified Examples

The examples on this page build from plain inspection into practical reuse.

Example 1. Read the whole base shape at once.

```cpp
auto print_shape = [](const auto& shape) {
  std::cout << "[";
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << shape[i];
  }
  std::cout << "]";
};

auto cube = pluss::make_series(
    {
        {
            {1.0, 2.0, 3.0, 4.0},
            {5.0, 6.0, 7.0, 8.0},
            {9.0, 10.0, 11.0, 12.0},
        },
        {
            {13.0, 14.0, 15.0, 16.0},
            {17.0, 18.0, 19.0, 20.0},
            {21.0, 22.0, 23.0, 24.0},
        },
    });

print_shape(cube.sizes());
```

### Output

```text
[2, 3, 4]
```

The order is rank order: axis `0`, then axis `1`, then axis `2`.

Example 2. `sizes()` reports the current logical interpretation after an axis
operation.

```cpp
print_shape(cube.transpose(0, 2).sizes());
```

### Output

```text
[4, 3, 2]
```

That matters because `sizes()` is not reporting the construction-time shape. It
is reporting the shape of the handle you are actually holding now.

Example 3. Scalars report an empty shape, not a fake `[1]`.

```cpp
auto scalar = pluss::make_series(42.0);
print_shape(scalar.sizes());
```

### Output

```text
[]
```

This is a real semantic distinction in `Series`: rank-`0` scalar and rank-`1`
length-`1` vector are different shapes.

Example 4. Rank-editing methods update the reported shape immediately.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

print_shape(mat.unsqueeze(1).sizes());
```

### Output

```text
[2, 1, 3]
```

That is why `sizes()` is such a practical teaching and debugging tool: it lets
you inspect the exact shape consequence of a structural method before moving on.

## Notes

- `sizes()` reports logical extents only.
- It does not expose strides, contiguity, dtype, or platform.
- Use [`size()`](size.md) when one axis is the real subject of the code.
- Use `sizes()` when the full shape is what you need to reason about or reuse.

## See Also

- [`Series::size`](size.md): inspect one axis at a time.
- [`Series::view`](view.md), [`Series::reshape`](reshape.md): consume shape-like
  ranges directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="view.html">
    <span class="label">Next</span>
    <span class="title">view</span>
  </a>
</div>
