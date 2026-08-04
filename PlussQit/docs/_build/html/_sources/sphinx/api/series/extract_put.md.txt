# Series::extract_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a modified copy of the series after writing through the full
`ExtractSpec` selector language.

This is the non-mutating sibling of [`extract_put_()`](extract_put_.md). It
accepts the same selectors, applies the same broadcast and cardinality rules,
and performs the same logical write. The only difference is ownership:

- `extract_put_()` mutates the current series
- `extract_put()` leaves the current series unchanged and returns a patched copy

Use this when selector-based patching is part of an expression and you do not
want to disturb the original value.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

All major `extract()`-style selector forms are supported here:

- scalar indices
- slices, including negative-step slices
- boolean masks
- integer index series
- `Ellipsis`
- `NewAxis`

Key write-side behavior:

- Scalar indices narrow the targeted destination region.
- Negative-step slices remain legal here.
- Advanced selectors are materialized internally into explicit destination
  positions before the write happens.
- `NewAxis` is accepted for selector compatibility, but it does not contribute a
  new writable destination axis.

**values** (`const Series&`)

The source data to write into the selected positions.

`values` may be:

- scalar, in which case it broadcasts across the whole selection
- non-scalar, in which case its element count must match the selection exactly

If dtype or platform differ, `values` is converted before assignment.

## Returns

`Series`
    A new series containing the write result.

## Examples

Example 1. Start from the already-patched grid produced by the in-place
examples, then show that `extract_put()` itself does not mutate that input.

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

Example 2. A scalar source broadcasts into the selected region of the returned
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

The original `grid` still holds its old right-hand columns. Only `patched`
contains the replacement.

Example 3. Advanced selectors work on the copy-returning surface too. Here a
boolean row mask zeros only the masked rows.

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

The same selector would have been legal on [`extract_put_()`](extract_put_.md);
the difference is only whether the update lands on `grid` itself or on the
returned series.

## Notes

- `extract_put()` is the unrestricted selector-write surface when you want value
  semantics rather than mutation.
- This is usually the cleanest method for "derive a patched copy" code.
- If you need the update to happen in place, use
  [`extract_put_()`](extract_put_.md).
- If you want the selector to be rejected unless it is view-representable, use
  [`traverse_put()`](traverse_put.md) instead.

## See Also

- [`Series::extract_put_`](extract_put_.md): the in-place form.
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
