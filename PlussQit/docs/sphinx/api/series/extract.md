# Series::extract

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::extract(pluss::msharp::RangeView::Of<ExtractSpec> specs) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2) const -> Series
Series::extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2, ExtractSpec a3) const -> Series
```

## What It Does

Applies a dynamic selector list to a `Series`.

This is the real general-purpose indexing surface for `Series`. It understands:

- scalar indices
- `SliceAll`, `SliceFrom`, `SliceTo`, `SliceRange`
- `Ellipsis`
- `NewAxis`
- advanced index `Series` of bool or integer dtype

If a selection can stay a cheap view, `extract()` takes that path. If not, it
materializes what it needs. That is why `extract()` can do things
[`slice()`](slice.md) deliberately refuses, such as negative-step reversal or
advanced-index selection.

## Parameters

**specs / a0..a3** (`ExtractSpec`)

The selector sequence.

Important normalization rules:

- if no `Ellipsis` is supplied, missing trailing axes are filled with
  `SliceAll()`
- at most one `Ellipsis` is allowed
- negative scalar indices resolve from the end
- bool advanced indices must be 1D and match the indexed axis length
- integer advanced indices are normalized to `Int64`, and negative values also
  resolve from the end

The fixed-arity overloads are just convenience wrappers around the main
`RangeView` form.

## Returns

`Series`
    The selected result, as either a view or a materialized series depending on
    what the selector requires.

## Examples

Example 1. A basic range extract can look a lot like `slice()`, but still lives
on the richer selector surface.

```cpp
auto slice_src = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

const std::vector<ExtractSpec> right_two{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(2, 4, 1)};

std::cout << slice_src.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(right_two));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[12, 13],
  [22, 23],
  [32, 33]]
```

Example 2. Unlike `slice()`, `extract()` can reverse an axis with a negative
step by materializing the requested order.

```cpp
const std::vector<ExtractSpec> reverse_cols{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};

std::cout << slice_src.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(reverse_cols));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[13, 12, 11, 10],
  [23, 22, 21, 20],
  [33, 32, 31, 30]]
```

Example 3. `NewAxis` inserts a size-1 dimension, while a scalar index still
consumes one existing axis.

```cpp
const std::vector<ExtractSpec> with_newaxis{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{1}};

std::cout << slice_src.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(with_newaxis));
```

### Output

```text
pluss::Series(shape=[1,3], strides=[12,4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[11, 21, 31]]
```

Example 4. A bool advanced index keeps only the rows whose mask entry is `true`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
const std::vector<ExtractSpec> bool_pick{
    mask,
    pluss::Slicing::SliceAll()};

std::cout << slice_src.extract(
    pluss::msharp::RangeView::Of<ExtractSpec>(bool_pick));
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [30, 31, 32, 33]]
```

## Notes

- `extract()` is the most expressive indexing entrypoint on the base `Series`
  surface.
- `slice()` is cheaper and simpler, but intentionally narrower.
- When no advanced index tensors are involved, [`index()`](index.md) often
  collapses back to `extract()` semantics anyway.

## See Also

- [`Series::index`](index.md): the Torch-style entrypoint, especially for
  advanced index tensors.
- [`Series::slice`](slice.md): cheap basic slicing only.

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
