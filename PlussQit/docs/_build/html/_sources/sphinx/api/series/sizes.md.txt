<!-- pluss-hand-authored -->

# Series::sizes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sizes() const -> pluss::msharp::RangeView::Int
```

## What It Does

Returns the full current shape as a lightweight borrowed view.

This is the bulk counterpart to [`size()`](size.md). Instead of asking for one
axis length at a time, you get the entire sequence of extents in rank order.

The important design choice here is that `sizes()` does **not** allocate a new
owning container just to hand you shape metadata. It returns
`pluss::msharp::RangeView::Int`, which is a non-owning view over the existing
shape storage. That makes `sizes()` cheap enough to use as plumbing between
shape-driven operations such as `view()`, `reshape()`, `expand()`, or other
factory calls that accept a shape-like span.

## Parameters

This method takes no parameters.

## Returns

`pluss::msharp::RangeView::Int`
    A borrowed view of the current axis extents.

## Lifetime Notes

Because the return value is borrowed, you should treat it as shape metadata tied
to the `Series` it came from. It is ideal for immediate inspection and for
passing directly into another call; it is not an owning snapshot meant to outlive
the source object arbitrarily.

## Examples

Example 1. Read the whole base shape at once.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{
        1,  2,  3,  4,
        5,  6,  7,  8,
        9,  10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24},
    {2, 3, 4});

auto shape = cube.sizes();

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

Example 2. `sizes()` tracks the current logical view immediately after a shape
operation.

```cpp
shape = cube.transpose(0, 2).sizes();

std::cout << "[";
for (std::size_t i = 0; i < shape.size(); ++i) {
  if (i != 0) std::cout << ", ";
  std::cout << shape[i];
}
std::cout << "]";
```

### Output

```text
[4, 3, 2]
```

The physical storage did not move there; only the logical interpretation
changed. `sizes()` reports the interpreted shape, not the original factory
layout.

## Notes

- `sizes()` is borrowed metadata, not an owning `std::vector<std::int64_t>`.
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
