<!-- pluss-hand-authored -->

# Series::extract

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::extract(pluss::msharp::RangeView::Of<ExtractSpec> specs) const -> Series
Series::extract(ExtractSpec a0) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2, ExtractSpec a3) const -> Series
```

## What It Does

Evaluates a heterogeneous selector list against a `Series`.

`extract()` is the broad, dynamic indexing surface. It is the method to reach for
when the selector itself is part of your program state instead of being hard-coded
into a simpler helper like [`slice()`](slice.md), [`pos()`](pos.md), or
[`block()`](block.md).

It understands all of the selector forms exposed by `ExtractSpec`:

- scalar indices
- slice selectors such as `SliceAll()`, `SliceFrom()`, `SliceTo()`, and `SliceRange()`
- `Ellipsis()`
- `NewAxis()`
- advanced index `Series` of bool or integer dtype

The important behavioral boundary is that `extract()` works **axis by axis**.
When a selector can stay a cheap view, it does. When it cannot, `extract()` materializes
the needed position list and continues. That is why `extract()` can express things
that [`traverse_slice()`](traverse_slice.md) deliberately rejects, such as negative-step
reversal and advanced index tensors.

## Parameters

**specs / a0..a3** (`ExtractSpec`)

The selector sequence.

Each selector kind changes rank traversal differently:

- A scalar index consumes one existing axis and removes it from the result.
- A slice consumes one existing axis and keeps it, possibly with a different length.
- `NewAxis()` inserts a new size-1 dimension and consumes no source axis.
- `Ellipsis()` expands to however many trailing `SliceAll()` selectors are needed.
- A bool advanced index consumes one axis and keeps only the positions where the mask is `true`.
- An integer advanced index consumes one axis and selects explicit positions on that axis.

Normalization rules:

- If you omit trailing axes, `extract()` fills them with `SliceAll()`.
- At most one `Ellipsis()` is allowed.
- Negative scalar indices count from the end of the indexed axis.
- Slice steps may be positive or negative, but may not be `0`.
- Bool advanced indices must be 1-D and match the indexed axis length.
- Integer advanced indices are normalized to `Int64`, and negative values count from the end.

The overloads differ only in how the selectors are supplied:

- The fixed-arity overloads are convenience wrappers for 1 to 4 selectors.
- The `RangeView` overload is the general form for dynamic selector lists and for lists longer than 4.

```{note} `extract()` is not the same advanced-index surface as [`index()`](index.md)
`extract()` applies advanced selectors one axis at a time. A shaped integer index
therefore becomes a flat position list on the axis being selected. If you want
Torch-style advanced indexing that preserves the advanced index tensor's own shape,
use [`index()`](index.md) instead.
```

## Returns

`Series`
    The selected result. Depending on the selector, this may be a view or a
    materialized series.

## Examples

Example 1. The 1-selector convenience overload is the shortest form when you are
only consuming one axis.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

std::cout << grid.extract(pluss::Slicing::SliceRange(0, 2, 1));
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23]]
```

Because only the first axis is mentioned, `extract()` implicitly appends
`SliceAll()` for the remaining axis.

Example 2. The 2-selector convenience overload is the natural form for ordinary
row/column slicing.

```cpp
std::cout << grid.extract(
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(2, 4, 1));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[12, 13],
  [22, 23],
  [32, 33]]
```

Example 3. `NewAxis()` inserts a size-1 dimension, while a scalar selector still
consumes the axis it indexes.

```cpp
std::cout << grid.extract(
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{1});
```

### Output

```text
pluss::Series(shape=[1,3], strides=[12,4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[11, 21, 31]]
```

The inserted axis survives in the result. The scalar column index does not.

Example 4. The 4-selector convenience overload is useful once the selector starts
mixing rank-changing and rank-preserving operations.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{
        1, 2, 3, 4, 5, 6,
        7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24},
    {2, 3, 4});

std::cout << cube.extract(
    std::int64_t{1},
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(0, 4, 2),
    pluss::Slicing::NewAxis());
```

### Output

```text
pluss::Series(shape=[3,2,1], strides=[4,2,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[[13],
  [15]],
  [[17],
  [19]],
  [[21],
  [23]]]
```

Example 5. A negative slice step is legal on `extract()`. This is one of the
places where it intentionally goes beyond [`traverse_slice()`](traverse_slice.md).

```cpp
const std::vector<ExtractSpec> reverse_cols{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};

std::cout << grid.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(reverse_cols));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[13, 12, 11, 10],
  [23, 22, 21, 20],
  [33, 32, 31, 30]]
```

Example 6. A bool advanced index acts as a 1-D mask over the consumed axis.

```cpp
auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
const std::vector<ExtractSpec> bool_pick{
    row_mask,
    pluss::Slicing::SliceAll()};

std::cout << grid.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(bool_pick));
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 7. A shaped integer advanced index is flattened by `extract()` because
the method materializes a single axis-worth of positions before continuing.

```cpp
auto row_pick_2d = pluss::make_series(std::vector<std::int64_t>{2, 0, 2, 1}, {2, 2});
const std::vector<ExtractSpec> shaped_int_pick{
    row_pick_2d,
    pluss::Slicing::SliceAll()};

std::cout << grid.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(shaped_int_pick));
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[30, 31, 32, 33],
  [10, 11, 12, 13],
  [30, 31, 32, 33],
  [20, 21, 22, 23]]
```

The source index tensor had shape `[2, 2]`, but `extract()` used its flattened
position list and replaced the row axis with length `4`. This is correct for
`extract()`, but it is exactly the case where [`index()`](index.md) behaves
differently.

## Notes

- Use `extract()` when the selector is heterogeneous, dynamic, or may need to materialize.
- Use [`traverse_slice()`](traverse_slice.md) when you want to enforce cheap-view-only traversal.
- Use [`index()`](index.md) when the main intent is Torch-style advanced indexing.

## See Also

- [`Series::index`](index.md): advanced-index-first routing, especially for integer index tensors.
- [`Series::traverse_slice`](traverse_slice.md): the view-only sibling surface.
- [`Series::extract_put`](extract_put.md): copy-returning write-side counterpart.
- [`Series::extract_put_`](extract_put_.md): in-place write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="block.html">
    <span class="label">Previous</span>
    <span class="title">block</span>
  </a>
  <a class="next-link" href="extract_put.html">
    <span class="label">Next</span>
    <span class="title">extract_put</span>
  </a>
</div>
