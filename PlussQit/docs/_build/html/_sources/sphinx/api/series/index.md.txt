<!-- pluss-hand-authored -->

# Series::index

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const -> Series
```

## What It Does

Evaluates a selector list with **advanced indexing as the primary intent**.

That sounds close to [`extract()`](extract.md), and a large part of the selector
vocabulary is shared. The difference is routing. `index()` tries to preserve
Torch-style advanced-index semantics whenever the selector list has the right
shape, and only falls back to [`extract()`](extract.md) when it cannot do so
without changing the meaning.

In practice, `index()` has three behavioral zones:

1. All selectors are advanced index tensors.
2. Advanced index tensors are mixed only with identity `SliceAll()` selectors.
3. Anything else.

Zones 1 and 2 stay on the dedicated advanced-index path. Zone 3 delegates to
[`extract()`](extract.md).

That boundary matters because the advanced-index path preserves the advanced
index tensor shape and zips multiple advanced tensors together. The `extract()`
fallback, by contrast, processes one axis at a time.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The selector sequence.

Selectors accepted by `index()` are the same `ExtractSpec` forms used by
[`extract()`](extract.md), but not every selector mix stays on the same internal route.

Advanced-index route:

- one or more advanced index `Series`
- optionally separated by identity `SliceAll()` selectors only
- omitted trailing axes are treated as `SliceAll()`

Fallback-to-`extract()` route:

- scalar indices
- non-identity slices such as `SliceRange(1, 4, 2)`
- `NewAxis()`
- any heterogeneous pattern that is not "advanced indices plus identity slices only"

Advanced index requirements:

- bool tensors act as masks
- integer tensors act as explicit positions
- negative integer values count from the end
- index tensors must live on the same platform as the source series

## Returns

`Series`
    The indexed result.

```{note} The easiest mental model
If you are thinking "I want Torch-style advanced indexing," start with `index()`.
If you are thinking "I want a general dynamic selector list," start with
[`extract()`](extract.md).
```

## Examples

Example 1. A single integer advanced index tensor selects rows and preserves the
index tensor as the front shape of the result.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});
auto row_pick = pluss::make_series(std::vector<std::int64_t>{2, 0, 2}, {3});

const std::vector<ExtractSpec> single_adv{row_pick};
std::cout << grid.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(single_adv));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[30, 31, 32, 33],
  [10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 2. Multiple advanced index tensors are zipped together when the only
other selector is identity `SliceAll()`.

```cpp
auto row_idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 1}, {4});
auto col_idx = pluss::make_series(std::vector<std::int64_t>{2, 3, 0, 2}, {4});
auto adv_cube = pluss::make_series(
    std::vector<double>{
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23},
    {3, 4, 2});

const std::vector<ExtractSpec> pair_specs{
    row_idx,
    col_idx,
    pluss::Slicing::SliceAll()};
std::cout << adv_cube.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(pair_specs));
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[4, 5],
  [14, 15],
  [16, 17],
  [12, 13]]
```

Each `(row_idx[n], col_idx[n])` pair chooses one lane from the first two axes.
`SliceAll()` then keeps the full trailing length-2 slice for that pair.

Example 3. A bool mask also rides the advanced-index route.

```cpp
auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
const std::vector<ExtractSpec> bool_index_specs{
    row_mask,
    pluss::Slicing::SliceAll()};

std::cout << grid.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(bool_index_specs));
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[10, 11, 12, 13],
  [30, 31, 32, 33]]
```

Example 4. Once the selector is ordinary slicing rather than advanced indexing,
`index()` effectively becomes a pass-through to [`extract()`](extract.md).

```cpp
const std::vector<ExtractSpec> alias_specs{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(1, 4, 2)};

std::cout << grid.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(alias_specs));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

Example 5. A non-identity slice inside a mixed advanced expression is the
precise boundary where `index()` falls back to `extract()`.

```cpp
const std::vector<ExtractSpec> mixed_specs{
    row_idx,
    pluss::Slicing::SliceRange(1, 4, 2),
    pluss::Slicing::SliceAll()};

std::cout << adv_cube.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(mixed_specs));
```

### Output

```text
pluss::Series(shape=[4,2,2], strides=[8,4,1], dtype=float64, platform=cpu, numel=16, contiguous=false)
data=[[[2, 3],
  [6, 7]],
  [[10, 11],
  [14, 15]],
  [[18, 19],
  [22, 23]],
  [[10, 11],
  [14, 15]]]
```

This is not the same semantics as Example 2. The second selector is no longer an
identity slice, so `index()` stops doing zipped advanced indexing and delegates to
[`extract()`](extract.md).

Example 6. A shaped integer index tensor keeps its own shape on the advanced path.

```cpp
auto row_pick_2d = pluss::make_series(std::vector<std::int64_t>{2, 0, 2, 1}, {2, 2});
const std::vector<ExtractSpec> shaped_index_specs{row_pick_2d};

std::cout << grid.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(shaped_index_specs));
```

### Output

```text
pluss::Series(shape=[2,2,4], strides=[8,4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[[30, 31, 32, 33],
  [10, 11, 12, 13]],
  [[30, 31, 32, 33],
  [20, 21, 22, 23]]]
```

This is the clearest contrast with [`extract()`](extract.md): the same shaped
integer index tensor produced a `[4, 4]` result there, but here it preserves its
own `[2, 2]` leading shape and appends the untouched trailing axis.

## Notes

- `index()` is the right entrypoint when advanced index tensors are the main story.
- `index()` and [`extract()`](extract.md) share selector syntax but not always the same semantics.
- Whether a mixed selector stays on the advanced path depends on the non-advanced selectors being exact identity `SliceAll()` operations.

## See Also

- [`Series::extract`](extract.md): the more general dynamic selector surface.
- [`Series::take`](take.md): flat linear indexing rather than axis-aware advanced indexing.
- [`Series::index_put`](index_put.md): copy-returning write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract_put_.html">
    <span class="label">Previous</span>
    <span class="title">extract_put_</span>
  </a>
  <a class="next-link" href="index_put.html">
    <span class="label">Next</span>
    <span class="title">index_put</span>
  </a>
</div>
