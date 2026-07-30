# Series::extract_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Mutates the current series by writing `values` into the region selected by
`specs`, then returns `*this`.

Conceptually, `extract_put_()` is "selector-based assignment" for the same
selector language documented on [`extract()`](extract.md). The implementation
normalizes the selector, materializes any non-view pieces into explicit per-axis
index series, and then routes the actual write through `index_put_()`. That is
why it can handle both easy cases like a whole column write and harder cases
like reverse-step slices or advanced masks.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The write selector.

Everything important about [`extract()`](extract.md) selector semantics carries
over here:

- scalar indices collapse one source axis
- identity slices preserve the full axis
- negative-step slices are legal
- bool/int advanced index series are legal
- `NewAxis` is accepted but does not create an extra writable destination axis

**values** (`const Series&`)

The replacement data.

`values` is first converted to the target dtype/platform if necessary, then must
satisfy one of two cardinality rules:

- `numel() == 1`, which broadcasts to the full selection
- `numel()` exactly equals the selected element count

If `values` is non-scalar and its element count does not match the selection,
the call throws.

## Returns

`Series&`
    The same mutated series handle.

## Examples

Example 1. A scalar source broadcasts across a full selected column.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

const std::vector<ExtractSpec> fill_column{
    pluss::Slicing::SliceAll(),
    std::int64_t{1}};

grid.extract_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(fill_column),
    pluss::codem::value(-1.0));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, 12, 13],
  [20, -1, 22, 23],
  [30, -1, 32, 33]]
```

Example 2. A negative-step slice is not a cheap view write, but
`extract_put_()` still handles it by materializing the indexed positions.

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

The selected positions are the row-1 columns visited by the reverse slice:
first column `3`, then column `1`. The replacement values are consumed in that
same logical order.

Example 3. `NewAxis` is accepted in the selector, but it does not create a new
writable target dimension. It is ignored for target addressing.

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

Example 4. A non-scalar source must match the selected element count exactly.

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

## Notes

- `extract_put_()` mutates in place. If you need copy-returning behavior, use
  [`extract_put()`](extract_put.md).
- For richer write selectors, `extract_put_()` is usually clearer than
  `index_put_()` because the selector itself documents the intended region.
- This surface accepts selectors that a pure view traversal cannot represent,
  such as reverse-step slices.

## See Also

- [`Series::extract_put`](extract_put.md): same selector language, but returns a
  modified copy.
- [`Series::index_put_`](index_put_.md): write through explicit index tensors or
  constrained convenience overloads.
- [`Series::extract`](extract.md): selector-based reads.

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
