<!-- pluss-hand-authored -->

# Series::traverse_slice

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_slice(pluss::msharp::RangeView::Of<ExtractSpec> specs) const -> Series
```

## What It Does

Builds a dynamic selector view, but only for selectors that remain
view-representable.

That restriction is the entire point of this method. `traverse_slice()` accepts
the dynamic `ExtractSpec` grammar used by [`extract()`](extract.md), but it only
permits the subset that can be lowered into ordinary view operations such as
`select()`, `slice()`, and `unsqueeze()`. If the selector would require
materializing gathered data, the method refuses it.

So it sits between two neighboring surfaces:

- more expressive than [`slice()`](slice.md), because it accepts a dynamic spec
  list
- stricter than [`extract()`](extract.md), because it stays in the aliasing view
  lane only

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The dynamic selector list.

The most important selector toggles are:

- scalar index: consumes one source axis and removes it from the result rank
- positive-step slice: keeps the axis but may make the result non-contiguous
- `SliceAll()`: preserves the axis unchanged
- `NewAxis()`: inserts a length-1 axis into the result
- `Ellipsis()`: expands to however many identity slices are needed

## Accepted vs Rejected Selectors

Accepted:

- scalar indices
- positive-step slices
- `SliceAll()`
- `NewAxis()`
- `Ellipsis()`, once normalized

Rejected:

- advanced integer or boolean index tensors
- reverse slices
- any slice with `step <= 0`

Those rejected cases belong on [`extract()`](extract.md), which is allowed to
materialize.

## Returns

`Series`
    A view onto the original storage.

## Gradient

`traverse_slice()` is differentiable for the same reason `slice()` and
`select()` are: it composes only view-representable operations. Backward
therefore routes sensitivity back to the original storage positions selected by
the normalized spec list.

## Examples

Example 1. Shared setup plus a mixed scalar-and-slice selector.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

const std::vector<ExtractSpec> row_stride{
    std::int64_t{1},
    pluss::Slicing::SliceRange(1, 4, 2)};

std::cout << grid.traverse_slice(
    pluss::msharp::RangeView::Of<ExtractSpec>(row_stride));
```

### Output

```text
pluss::Series(shape=[2], strides=[2], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[21, 23]
```

The scalar row index removes the row axis; the stepped column slice keeps a
single remaining axis with stride `2`.

Example 2. `NewAxis()` inserts a real length-1 axis into the result.

```cpp
const std::vector<ExtractSpec> newaxis_column{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{2}};

std::cout << grid.traverse_slice(
    pluss::msharp::RangeView::Of<ExtractSpec>(newaxis_column));
```

### Output

```text
pluss::Series(shape=[1,3], strides=[12,4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[12, 22, 32]]
```

Example 3. `Ellipsis()` expands into however many identity slices are needed.

```cpp
const std::vector<ExtractSpec> ellipsis_column{
    pluss::Slicing::Ellipsis(),
    std::int64_t{2}};

std::cout << grid.traverse_slice(
    pluss::msharp::RangeView::Of<ExtractSpec>(ellipsis_column));
```

### Output

```text
pluss::Series(shape=[3], strides=[4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[12, 22, 32]
```

On this 2D input, `Ellipsis(), 2` normalizes to the same logical effect as
`SliceAll(), 2`.

Example 4. Advanced index tensors are rejected.

```cpp
try {
  auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
  const std::vector<ExtractSpec> advanced_specs{
      row_mask,
      pluss::Slicing::SliceAll()};

  std::cout << grid.traverse_slice(
      pluss::msharp::RangeView::Of<ExtractSpec>(advanced_specs)) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_slice: selector is not view-representable; use extract() or extract_put_() for materializing paths
```

Example 5. Reverse slices are rejected for the same reason.

```cpp
try {
  const std::vector<ExtractSpec> reverse_specs{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};

  std::cout << grid.traverse_slice(
      pluss::msharp::RangeView::Of<ExtractSpec>(reverse_specs)) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_slice: selector is not view-representable; use extract() or extract_put_() for materializing paths
```

Example 6. Backward marks exactly the storage positions selected by the dynamic
spec list.

```cpp
auto tracked_grid = pluss::make_series(
    std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
    {3, 4});
tracked_grid.tracks_slope_(true);

tracked_grid.traverse_slice(
    pluss::msharp::RangeView::Of<ExtractSpec>(row_stride)).sum().reverse();
std::cout << tracked_grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [0, 1, 0, 1],
  [0, 0, 0, 0]]
```

Only row `1`, columns `1` and `3` were selected, so only those positions
receive gradient.

## Notes

- `traverse_slice()` is a view builder, not a general gather engine.
- If there is any realistic chance the selector needs advanced indexing or
  reversed traversal, start with [`extract()`](extract.md) instead.
- For writes through the same view-representable subset, use
  [`traverse_put_()`](traverse_put_.md) or [`traverse_put()`](traverse_put.md).

## See Also

- [`Series::slice`](slice.md): the fixed-shape basic slicing helper.
- [`Series::extract`](extract.md): the full dynamic selector surface.
- [`Series::traverse_put_`](traverse_put_.md): in-place writes through the same
  normalized view-only selector subset.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="traverse_put_.html">
    <span class="label">Next</span>
    <span class="title">traverse_put_</span>
  </a>
</div>
