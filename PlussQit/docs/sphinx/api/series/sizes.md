<!-- pluss-hand-authored -->

# Series::sizes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sizes() const -> pluss::msharp::RangeView::Int
```

## What It Does

Returns the full current shape as a lightweight borrowed view.

This is the shape-wide counterpart to [`size()`](size.md). Instead of asking
for one axis at a time, you receive the entire rank-ordered shape in one call.

The important behavior is not just "you get the sizes", but "you get them
cheaply." `sizes()` does not build a fresh owning container just to report
metadata. It exposes a borrowed view over the `Series`’s existing shape
storage, which makes it useful both for inspection and for wiring shape data
directly into other APIs such as [`view()`](view.md) or
[`reshape()`](reshape.md).

## Parameters

This method takes no parameters.

## Returns

`pluss::msharp::RangeView::Int`
    A borrowed view of the current axis extents.

## Lifetime Notes

Because the return value is borrowed, treat it as metadata tied to the `Series`
it came from. It is ideal for immediate inspection and for passing directly
into another method call; it is not an owning snapshot meant to be stored
indefinitely after the source object changes or disappears.

## Examples

The examples on this page build incrementally.

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

Example 2. `sizes()` tracks the current logical view immediately after a shape
operation.

```cpp
std::cout << "";
print_shape(cube.transpose(0, 2).sizes());
```

### Output

```text
[4, 3, 2]
```

The physical storage did not move there. Only the logical interpretation
changed, and `sizes()` reports that interpreted shape immediately.

Example 3. Scalars report an empty shape, not a fake length-`1` vector shape.

```cpp
auto scalar = pluss::make_series(42.0);
std::cout << "";
print_shape(scalar.sizes());
```

### Output

```text
[]
```

## Notes

- `sizes()` is borrowed metadata, not an owning snapshot.
- If you only need one axis, [`size()`](size.md) is the clearer API.
- `sizes()` reports extents only. It does not expose strides or contiguity.

## See Also

- [`Series::size`](size.md): query one axis.
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
