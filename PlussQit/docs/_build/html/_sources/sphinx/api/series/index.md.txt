# Series::index

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const -> Series
```

## What It Does

Evaluates a Torch-style indexing expression from a selector list.

At first glance that sounds identical to [`extract()`](extract.md), and for many
cases it is. The important distinction is that `index()` has special routing for
advanced index tensors:

- if every supplied selector is an advanced index tensor, `index()` uses the
  dedicated advanced-index path
- if advanced indices are mixed only with identity `SliceAll()` selectors,
  `index()` still keeps that advanced-index path
- otherwise it falls back to [`extract()`](extract.md)

So `index()` is mainly about advanced-index intent. For ordinary scalar/slice
selectors, `extract()` is the more explicit description of what is happening.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The selector sequence.

On this surface, the most important selector category is the advanced index
`Series`:

- bool advanced indices act as masks
- integer advanced indices pick explicit positions
- multiple advanced indices are combined in the advanced-index route when the
  surrounding non-advanced specs are all identity slices

If the selector list is not an "advanced-only" or "advanced plus identity
SliceAll" shape, `index()` delegates back to [`extract()`](extract.md).

## Returns

`Series`
    The indexed result.

## Examples

Example 1. Two integer index tensors plus `SliceAll()` form the dedicated
advanced-index path.

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

Each `(row_idx[n], col_idx[n])` pair chooses one lane from the first two axes,
and `SliceAll()` keeps the entire trailing length-2 lane.

Example 2. With only ordinary slices, `index()` is effectively an alias for
[`extract()`](extract.md).

```cpp
auto slice_src = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

const std::vector<ExtractSpec> alias_specs{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(1, 4, 2)};

std::cout << slice_src.index(
    pluss::msharp::RangeView::Of<ExtractSpec>(alias_specs));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

Example 3. Once a non-identity slice enters a mixed advanced-index expression,
`index()` falls back to the `extract()` path instead of the zipped
advanced-index route.

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

This result is materially different from Example 1 because the selector is no
longer "advanced indices plus identity slices only." That is the boundary where
`index()` stops using its dedicated advanced-index route and delegates to
[`extract()`](extract.md).

## Notes

- `index()` matters most when advanced index tensors are involved.
- For pure slice/scalar/Ellipsis/NewAxis work, [`extract()`](extract.md) usually
  communicates intent more directly.
- Mixed advanced selectors are not all treated the same; whether the
  non-advanced parts are identity `SliceAll()` selectors changes the route.

## See Also

- [`Series::extract`](extract.md): the general dynamic selector surface.
- [`Series::pos`](pos.md): scalar-only selection when you know the exact rank.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract_put_.html">
    <span class="label">Previous</span>
    <span class="title">extract_put_</span>
  </a>
  <a class="next-link" href="index_put_.html">
    <span class="label">Next</span>
    <span class="title">index_put_</span>
  </a>
</div>
