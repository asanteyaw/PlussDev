# Series::extract_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a modified copy of the series after writing `values` into the region
selected by `specs`.

This is the non-mutating companion to [`extract_put_()`](extract_put_.md). The
selector language is the same as [`extract()`](extract.md): scalar indices,
slices, `SliceAll()`, negative-step slices, bool/integer advanced index series,
`Ellipsis`, and `NewAxis`.

The important semantic boundary is simple:

- `extract()` reads from a selector
- `extract_put()` writes through a selector but leaves the original unchanged
- `extract_put_()` writes through a selector in place

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The write selector.

The same normalization rules used by [`extract()`](extract.md) apply here. In
particular:

- scalar indices collapse one source axis
- identity slices preserve the full axis
- negative-step slices are allowed
- advanced bool/int index series are materialized internally into explicit index
  positions
- `NewAxis` does not create a writable destination axis; it is ignored for
  target addressing because it consumes no source dimension

**values** (`const Series&`)

The data to write.

`values` may be:

- a scalar, which broadcasts across the selected region
- a non-scalar whose `numel()` exactly matches the selected element count

Before writing, `values` is converted to the destination dtype/platform if
needed.

## Returns

`Series`
    A new `Series` with the requested update applied.

## Examples

Example 1. The original input stays unchanged until you choose to use the
returned copy.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{100, -1, 12, 13, 200, 70, 22, 90, 300, -1, 32, 33},
    {3, 4});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

Example 2. A scalar source broadcasts across the selected region in the returned
copy.

```cpp
const std::vector<ExtractSpec> right_two{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(2, 4, 1)};

auto patched = grid.extract_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(right_two),
    pluss::codem::value(5.0));

std::cout << patched;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 5, 5],
  [200, 70, 5, 5],
  [300, -1, 5, 5]]
```

Example 3. Advanced selectors work here too. A boolean row mask plus scalar
broadcast can zero only the masked rows.

```cpp
auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});

const std::vector<ExtractSpec> mask_specs{
    row_mask,
    pluss::Slicing::SliceAll()};

std::cout << grid.extract_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(mask_specs),
    pluss::codem::value(0.0));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [200, 70, 22, 90],
  [0, 0, 0, 0]]
```

## Notes

- `extract_put()` is the copy-returning write path. If you want mutation, use
  [`extract_put_()`](extract_put_.md).
- Non-scalar `values` are matched by element count, not by preserving a
  higher-rank source shape contract.
- `NewAxis` in a write selector is accepted, but it does not add a writable
  target dimension.

## See Also

- [`Series::extract_put_`](extract_put_.md): the in-place form.
- [`Series::extract`](extract.md): read from the same selector language.
- [`Series::index_put_`](index_put_.md): write through explicit index tensors or
  convenience index overloads instead of `ExtractSpec` selectors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract.html">
    <span class="label">Previous</span>
    <span class="title">extract</span>
  </a>
  <a class="next-link" href="extract_put_.html">
    <span class="label">Next</span>
    <span class="title">extract_put_</span>
  </a>
</div>
