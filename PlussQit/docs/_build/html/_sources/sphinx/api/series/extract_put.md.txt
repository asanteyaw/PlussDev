<!-- pluss-hand-authored -->

# Series::extract_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a patched copy of the series after writing through the full
`ExtractSpec` selector language.

This is the value-semantic sibling of [`extract_put_()`](extract_put_.md). The
selector grammar is the same, the cardinality rules are the same, and the
logical write order is the same. The only difference is where the update lands:

- `extract_put_()` mutates the current series
- `extract_put()` leaves the current series unchanged and returns a new one

That makes `extract_put()` the right surface when the write is part of an
expression, a pipeline, or any other place where "derive a modified result"
reads better than "mutate this object first, then keep going."

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

This method accepts the same broad selector family as [`extract()`](extract.md):

- scalar indices
- positive or negative-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`
- bool mask series
- integer index series

The write-side meaning is worth stating carefully:

- scalar indices narrow the destination region
- slices preserve an axis and choose which positions on that axis will be
  written
- `Ellipsis()` expands to the required number of identity slices
- advanced selectors are materialized into explicit destination positions before
  assignment
- `NewAxis()` is accepted for selector compatibility, but it does not create a
  new writable destination axis

**values** (`const Series&`)

The source values that will be written into the selected destination positions.

Two cardinality modes are supported:

- scalar source: broadcast across the entire selection
- non-scalar source: `values.numel()` must exactly match the number of selected
  destination elements

For non-scalar writes, matching is by element count, not by preserving the
original shape of `values`. The source is consumed in the selector's logical
visit order.

## Returns

`Series`
    A new series containing the write result.

## Examples

Example 1. Shared setup. Start from a grid whose state is easy to track by eye.

```cpp
auto grid = pluss::make_series(
    {100.0, -1.0, 12.0, 13.0,
     200.0, 70.0, 22.0, 90.0,
     300.0, -1.0, 32.0, 33.0},
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

Example 2. A scalar source broadcasts across the selected region of the
returned copy.

```cpp
const auto right_two = std::vector<ExtractSpec>{
    pluss::Slicing::SliceAll(),
    pluss::Slicing::SliceRange(2, 4, 1)};

auto patched = grid.extract_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(right_two),
    pluss::make_series(5.0));

std::cout << patched;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 5, 5],
  [200, 70, 5, 5],
  [300, -1, 5, 5]]
```

The selection is "all rows, columns `2:4`". Because the source is scalar, every
selected position receives the same replacement value.

Example 3. The original series is unchanged after the copy-returning write.

```cpp
std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

That is the practical difference between `extract_put()` and
[`extract_put_()`](extract_put_.md): same selector power, different ownership
result.

Example 4. Advanced selectors work here too. A bool row mask can patch only the
rows that satisfy the mask.

```cpp
auto row_mask = pluss::make_series({true, false, true}, {3});

const auto mask_specs = std::vector<ExtractSpec>{
    row_mask,
    pluss::Slicing::SliceAll()};

std::cout << grid.extract_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(mask_specs),
    pluss::make_series(0.0));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [200, 70, 22, 90],
  [0, 0, 0, 0]]
```

The mask is not "view-like," but `extract_put()` is allowed to materialize the
selected destinations, so the write still succeeds.

Example 5. A reverse slice is also legal on the copy-returning surface.

```cpp
const auto reverse_write = std::vector<ExtractSpec>{
    std::int64_t{1},
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};

std::cout << grid.extract_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(reverse_write),
    pluss::make_series({900.0, 700.0}, {2}));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 700, 22, 900],
  [300, -1, 32, 33]]
```

The selector visits row `1`, columns `3` then `1`. The source values are
consumed in exactly that order.

## Notes

- `extract_put()` is the broadest copy-returning selector-write surface on
  `Series`.
- Use it when mutation would be awkward or misleading, but the selector still
  needs full `extract()`-style power.
- If you want the same selector language in place, use
  [`extract_put_()`](extract_put_.md).
- If you want the method to reject anything that is not a plain traversal/view,
  use [`traverse_put()`](traverse_put.md).

## See Also

- [`Series::extract_put_`](extract_put_.md): the in-place sibling.
- [`Series::traverse_put`](traverse_put.md): copy-returning writes with a
  stricter selector gate.
- [`Series::extract`](extract.md): read through the same selector language.

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
