# Series::traverse_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current series through a selector that must remain
view-representable, mutates in place, and returns `*this`.

This is the strict write-side companion to [`traverse_slice()`](traverse_slice.md).
It uses selector syntax, but it does not accept the full `extract()` surface.
Its contract is intentionally narrower:

- destination selection must be representable as an ordinary view traversal
- after that selector check passes, writing follows the usual scalar-broadcast
  or exact-cardinality rules

So `traverse_put_()` is best when you want selector-based writes, but you also
want the API to reject anything that would require materializing scattered
positions.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

Accepted selectors are the same view-representable subset used by
[`traverse_slice()`](traverse_slice.md):

- scalar indices
- positive-step slices
- `SliceAll()`
- `Ellipsis`
- `NewAxis`

Rejected selectors include:

- negative-step slices
- boolean masks
- integer advanced index series
- mixed selectors that require materialized advanced indexing

`NewAxis` is still permitted because it is view-like bookkeeping rather than a
materializing index operation.

**values** (`const Series&`)

The source data to write into the selected region.

Two value modes are accepted:

- scalar broadcast
- exact element-count replacement

Like the broader selector-write surfaces, `values` is converted to the target
dtype and platform if necessary before assignment.

## Returns

`Series&`
    The same series, after mutation.

## Examples

Example 1. A whole-column write is accepted because the destination is plainly
view-representable.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

auto col_values = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
const std::vector<ExtractSpec> middle_column{
    pluss::Slicing::SliceAll(),
    std::int64_t{1}};

grid.traverse_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(middle_column),
    col_values);

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, 32, 33]]
```

Example 2. Scalar broadcast still works after the selector passes the stricter
view-only validator.

```cpp
const std::vector<ExtractSpec> row_tail{
    std::int64_t{2},
    pluss::Slicing::SliceRange(2, 4, 1)};

grid.traverse_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(row_tail),
    pluss::make_series(-5.0));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, -5, -5]]
```

Example 3. A reverse slice is rejected here even though the broader
[`extract_put_()`](extract_put_.md) surface could have handled it.

```cpp
try {
  const std::vector<ExtractSpec> reverse_specs{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};

  grid.traverse_put_(
      pluss::msharp::RangeView::Of<ExtractSpec>(reverse_specs),
      pluss::make_series(0.0));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_put_: selector is not view-representable; use extract() or extract_put_() for materializing paths
```

This is the key boundary of the method: it is not "write through any selector,"
it is "write through a selector only if that selector is still a pure
traversal."

## Notes

- `traverse_put_()` is the strictest selector-write method on `Series`.
- Use it when you want the API to guard against accidental advanced indexing.
- If the selector needs reverse traversal, masking, or integer gather-style
  addressing, move to [`extract_put_()`](extract_put_.md).
- If you want value semantics instead of mutation, use
  [`traverse_put()`](traverse_put.md).

## See Also

- [`Series::traverse_put`](traverse_put.md): copy-returning version with the
  same selector gate.
- [`Series::traverse_slice`](traverse_slice.md): read-side companion.
- [`Series::extract_put_`](extract_put_.md): broader write surface that accepts
  materializing selectors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="traverse_slice.html">
    <span class="label">Previous</span>
    <span class="title">traverse_slice</span>
  </a>
  <a class="next-link" href="traverse_put.html">
    <span class="label">Next</span>
    <span class="title">traverse_put</span>
  </a>
</div>
