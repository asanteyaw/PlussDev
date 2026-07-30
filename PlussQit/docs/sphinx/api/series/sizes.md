# Series::sizes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sizes() const -> pluss::msharp::RangeView::Int
```

## What It Does

Returns the full current shape as a lightweight borrowed view.

This is the bulk form of [`size()`](size.md): instead of asking for one axis at a
time, you receive the entire shape sequence. The return type is intentionally
cheap. `sizes()` does not allocate a new `std::vector<std::int64_t>` just to
hand you the shape; it exposes the existing shape metadata through
`pluss::msharp::RangeView::Int`.

That makes `sizes()` the right API when you want to pass a shape directly into
other Series operations such as `reshape`, `expand`, or shape-driven factory
calls.

## Parameters

This method takes no parameters.

## Returns

`pluss::msharp::RangeView::Int`
    A non-owning view over the current axis extents.

## Examples

Example 1. Borrow the base shape directly.

```cpp
auto shape_src = pluss::make_series(
    std::vector<double>{
        1, 2, 3, 4, 5, 6,
        7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24},
    {2, 3, 4});

auto shape = shape_src.sizes();

std::cout << "[";
for (std::size_t i = 0; i < shape.size(); ++i) {
  if (i != 0) std::cout << ", ";
  std::cout << shape[i];
}
std::cout << "]";
```

### Output

```text
[2, 3, 4]
```

Example 2. Because `sizes()` reports the current logical view, a transpose
changes the shape it exposes immediately.

```cpp
auto transposed = shape_src.transpose(0, 1);
auto shape = transposed.sizes();

std::cout << "[";
for (std::size_t i = 0; i < shape.size(); ++i) {
  if (i != 0) std::cout << ", ";
  std::cout << shape[i];
}
std::cout << "]";
```

### Output

```text
[3, 2, 4]
```

## Notes

- `sizes()` is a borrowed view, not an owning container.
- If you only need one axis, [`size()`](size.md) is clearer.
- `sizes()` reports shape, not strides. For layout details, use `strides()`.

## See Also

- [`Series::size`](size.md): axis-by-axis shape inspection.
- [`Series::view`](view.md): consume a shape-like sequence to reinterpret a
  contiguous layout.

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
