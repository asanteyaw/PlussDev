# Series::extract_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current series through the full `ExtractSpec` selector language,
mutates in place, and returns `*this`.

This is the write-side counterpart to [`extract()`](extract.md). If
[`traverse_put_()`](traverse_put_.md) is the strict "only write through a
plain view" surface, `extract_put_()` is the broader surface that accepts
selectors requiring materialization: reverse slices, boolean masks, integer
index series, and mixed selector forms.

The mental model is:

- `extract()` selects values
- `extract_put_()` selects destinations
- the selected destinations are then filled from `values`

Internally, non-trivial selectors are normalized into explicit index series and
the final update is routed through `index_put_()`. That is why this method can
write through selector shapes that are not representable as a simple chained
view.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

This method accepts the same broad selector language documented on
[`extract()`](extract.md), including:

- scalar integer indices
- `SliceAll()`
- bounded and unbounded slice ranges
- negative-step slices
- boolean mask series
- integer index series
- `Ellipsis`
- `NewAxis`

Important details:

- Scalar indices consume one source axis and target a narrower destination.
- Identity slices preserve the full addressed axis.
- Negative-step slices are legal here, even though they are rejected by
  [`traverse_put_()`](traverse_put_.md).
- Boolean and integer advanced selectors are legal here.
- `NewAxis` is accepted for selector compatibility, but it does not create a
  new writable axis. It is ignored for destination addressing because it
  consumes no source dimension.

**values** (`const Series&`)

The source data to assign into the selected destination positions.

`values` follows two cardinality rules:

- If `values.numel() == 1`, it broadcasts across the full selection.
- Otherwise `values.numel()` must equal the selected element count exactly.

Before writing, `values` is converted to the destination platform and dtype if
needed. Matching is by element count, not by preserving the original shape of
`values`; non-scalar sources are flattened to the selected logical order before
the write is applied.

## Returns

`Series&`
    The same series, after mutation.

## Examples

Example 1. A scalar source broadcasts across an entire selected column.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

const std::vector<ExtractSpec> fill_column{
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

This is the simplest successful case: the selector names three destination
positions, and a scalar source is replicated into each one.

Example 2. A reverse slice is legal here, so the write follows the selector's
logical visitation order rather than physical storage order.

```cpp
const std::vector<ExtractSpec> reverse_write{
    std::int64_t{1},
    pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};

auto reverse_values = pluss::make_series(std::vector<double>{90, 70}, {2});

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(reverse_write),
    reverse_values);

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, 12, 13],
  [20, 70, 22, 90],
  [30, -1, 32, 33]]
```

The selector visits row `1`, columns `3` then `1`. The two source elements are
consumed in exactly that order, so `90` lands in column `3` and `70` lands in
column `1`.

Example 3. `NewAxis` may appear in the selector, but it does not add a new
writeable destination dimension.

```cpp
const std::vector<ExtractSpec> with_newaxis{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{0}};

auto first_col = pluss::make_series(std::vector<double>{100, 200, 300}, {3});

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(with_newaxis),
    first_col);

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

Semantically this still means "write into column `0` of every row." `NewAxis`
does not create a phantom target axis that `values` must somehow match.

Example 4. Non-scalar sources must match the selected element count exactly.

```cpp
try {
  const std::vector<ExtractSpec> mismatch_specs{
      std::int64_t{0},
      pluss::Slicing::SliceRange(0, 4, 2)};

  auto mismatch_values = pluss::make_series(std::vector<double>{1, 2, 3}, {3});

  grid.extract_put_(
      pluss::msharp::RangeView::Of<ExtractSpec>(mismatch_specs),
      mismatch_values);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::extract_put_: source must be scalar or match selected element count
```

The selector picks only two destination positions, so a three-element
replacement series is invalid.

## Notes

- `extract_put_()` is the richest selector-write surface on `Series`.
- If the selector is naturally view-like and you want that restriction enforced
  up front, prefer [`traverse_put_()`](traverse_put_.md).
- If you want the same selector power without mutating the current object, use
  [`extract_put()`](extract_put.md).
- For flat or explicit index-list writes, [`index_put_()`](index_put_.md) can
  be more direct, but for most readable N-D region writes `extract_put_()` is
  the clearer API.

## See Also

- [`Series::extract_put`](extract_put.md): same selector language, but returns a
  modified copy.
- [`Series::traverse_put_`](traverse_put_.md): stricter write surface that
  accepts only view-representable selectors.
- [`Series::extract`](extract.md): read through the same selector language.
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
