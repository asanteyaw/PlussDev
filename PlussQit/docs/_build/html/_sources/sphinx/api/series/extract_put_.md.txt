<!-- pluss-hand-authored -->

# Series::extract_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current series through the full `ExtractSpec` selector
language, mutates in place, and returns `*this`.

This is the broadest selector-based write surface on `Series`. If
[`traverse_put_()`](traverse_put_.md) is the strict "only accept selectors that
can remain a pure view" method, `extract_put_()` is the method that says
"accept the full selector grammar and materialize positions when necessary."

That distinction matters in practice:

- reverse slices are legal here
- bool masks are legal here
- integer advanced indices are legal here
- mixed selectors that cannot stay a plain view are still legal here

Internally, the selector is normalized, the destination positions are computed,
and the final write routes through the explicit index-write machinery. So the
mental model is simple even if the lowering path is not:

- `extract()` chooses values to read
- `extract_put_()` chooses positions to write

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

This method accepts the same selector language documented for
[`extract()`](extract.md), including:

- scalar integer indices
- bounded or unbounded slices
- negative-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`
- bool mask series
- integer index series

Write-side selector details:

- scalar indices consume an existing axis and narrow the addressed region
- slices keep the axis and choose which positions on it will be written
- `Ellipsis()` expands to however many identity slices are required
- `NewAxis()` is accepted, but it does not create a new writable axis
- advanced selectors are materialized into explicit destination positions

**values** (`const Series&`)

The source values to assign into the selected destination positions.

Two cardinality rules apply:

- if `values.numel() == 1`, the scalar source broadcasts across the entire
  selection
- otherwise `values.numel()` must exactly match the number of selected
  destination elements

For non-scalar writes, matching is by element count. The source is consumed in
the selector's logical visit order rather than by trying to preserve its
original shape.

## Returns

`Series&`
    The same series, after mutation.

## Examples

Example 1. Shared setup. Start with an ordinary `3 x 4` grid.

```cpp
auto grid = pluss::make_series(
    {10.0, 11.0, 12.0, 13.0,
     20.0, 21.0, 22.0, 23.0,
     30.0, 31.0, 32.0, 33.0},
    {3, 4});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 33]]
```

Example 2. A scalar source broadcasts across all selected destinations.

```cpp
const auto fill_column = std::vector<ExtractSpec>{
    pluss::Slicing::SliceAll(),
    std::int64_t{1}};

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(fill_column),
    pluss::make_series(-1.0));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, 12, 13],
  [20, -1, 22, 23],
  [30, -1, 32, 33]]
```

Example 3. Reverse slices are legal here, so write order follows the selector's
logical visitation order.

```cpp
const auto reverse_write = std::vector<ExtractSpec>{
    std::int64_t{1},
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(reverse_write),
    pluss::make_series({90.0, 70.0}, {2}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, 12, 13],
  [20, 70, 22, 90],
  [30, -1, 32, 33]]
```

The selector visits row `1`, columns `3` then `1`, so `90` lands in column `3`
and `70` lands in column `1`.

Example 4. `NewAxis()` is accepted for compatibility with the selector grammar,
but it does not create a new writable destination dimension.

```cpp
const auto with_newaxis = std::vector<ExtractSpec>{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{0}};

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(with_newaxis),
    pluss::make_series({100.0, 200.0, 300.0}, {3}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

Semantically this is still just "write column `0` for every row."

Example 5. Bool masks are also legal here because `extract_put_()` is allowed
to materialize the addressed positions.

```cpp
auto row_mask = pluss::make_series({true, false, true}, {3});

const auto mask_specs = std::vector<ExtractSpec>{
    row_mask,
    pluss::Slicing::SliceAll()};

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(mask_specs),
    pluss::make_series(0.0));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [200, 70, 22, 90],
  [0, 0, 0, 0]]
```

Example 6. Non-scalar sources must match the selected element count exactly.

```cpp
try {
  const auto mismatch_specs = std::vector<ExtractSpec>{
      std::int64_t{0},
      pluss::Slicing::SliceRange(0, 4, 2)};

  grid.extract_put_(
      pluss::msharp::RangeView::Of<ExtractSpec>(mismatch_specs),
      pluss::make_series({1.0, 2.0, 3.0}, {3}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::extract_put_: source must be scalar or match selected element count
```

The selector names only two destination positions, so a three-element source is
rejected.

## Notes

- `extract_put_()` is the richest selector-write surface on `Series`.
- Use it when the selector may involve masks, reverse traversal, or other
  non-view addressing.
- If you want the same write to return a patched copy, use
  [`extract_put()`](extract_put.md).
- If you want the method to reject anything that is not view-representable, use
  [`traverse_put_()`](traverse_put_.md).

## See Also

- [`Series::extract_put`](extract_put.md): same selector language, but
  copy-returning.
- [`Series::traverse_put_`](traverse_put_.md): stricter in-place writes through
  the view-representable subset only.
- [`Series::extract`](extract.md): read-side counterpart.
- [`Series::index_put_`](index_put_.md): explicit index-list writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract_put.html">
    <span class="label">Previous</span>
    <span class="title">extract_put</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
