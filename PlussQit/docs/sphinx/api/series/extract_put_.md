<!-- pluss-hand-authored -->

# Series::extract_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put_(pluss::RangeView<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current `Series` through the full selector language, mutates in
place, and returns the same handle.

This is the broadest selector-based write surface on `Series`. If
[`traverse_put_()`](traverse_put_.md) is the strict method that only accepts
selectors that stay view-representable, `extract_put_()` is the method that
says "accept the richer selector language and materialize destination positions
when necessary."

That distinction matters in practice. This surface accepts things like:

- reverse slices
- bool masks
- integer index `Series`
- mixed selectors that cannot stay a plain view

The most useful mental model is simple:

- [`extract()`](extract.md) chooses values to read
- `extract_put_()` chooses positions to write

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The destination selector.

This method accepts the same broad selector family taught on
[`extract()`](extract.md), including:

- scalar integer indices
- bounded or unbounded slices
- negative-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`
- bool mask `Series`
- integer index `Series`

Write-side details are important here:

- scalar indices consume an existing axis and narrow the addressed region
- slices keep an axis and choose positions on it
- `Ellipsis()` expands to however many identity slices are required
- `NewAxis()` is accepted for selector compatibility, but it does not create a
  new writable axis
- advanced selectors are materialized into explicit destination positions

**values** (`const Series&`)

The source values to assign into the selected destination positions.

Two cardinality rules apply:

- if `values.numel() == 1`, the scalar source broadcasts across the entire
  selection
- otherwise `values.numel()` must exactly match the number of selected
  destination elements

For non-scalar writes, matching is by selected element count. The original
shape of `values` does not need to mirror the destination shape as long as the
write cardinality is correct.

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. Shared setup. Start from a simple `3 x 4` grid.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 33]]
```

Example 2. A scalar source broadcasts across every selected destination.

```cpp
grid.extract_put_(
    {pluss::Slicing::SliceAll(), std::int64_t{1}},
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

Example 3. Reverse slices are legal here, so write order follows the selector's
logical visitation order.

```cpp
grid.extract_put_(
    {std::int64_t{1}, pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)},
    pluss::make_series({90.0, 70.0}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, 12, 13],
  [20, 70, 22, 90],
  [30, -1, 32, 33]]
```

The selector visits row `1`, columns `3` then `1`, so `90` lands in column `3`
and `70` lands in column `1`.

Example 4. `NewAxis()` is accepted for selector compatibility, but it does not
create a new writable destination dimension.

```cpp
grid.extract_put_(
    {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
    pluss::make_series({100.0, 200.0, 300.0}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

Semantically, this is still just "write column `0` for every row."

Example 5. Bool masks are legal here because `extract_put_()` is allowed to
materialize destination positions.

```cpp
auto row_mask = pluss::make_series({true, false, true});

grid.extract_put_(
    {row_mask, pluss::Slicing::SliceAll()},
    pluss::make_series(0.0));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [200, 70, 22, 90],
  [0, 0, 0, 0]]
```

Example 6. Integer index `Series` selectors can target explicit rows in the
same write family.

```cpp
auto picked_rows = pluss::make_series({2LL, 0LL});

grid.extract_put_(
    {picked_rows, pluss::Slicing::SliceAll()},
    pluss::make_series({
        {3000.0, 3001.0, 3002.0, 3003.0},
        {1000.0, 1001.0, 1002.0, 1003.0},
    }));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1000, 1001, 1002, 1003],
  [200, 70, 22, 90],
  [3000, 3001, 3002, 3003]]
```

Example 7. `Ellipsis()` also works on the write side and expands to the missing
identity slices before the final write.

```cpp
grid.extract_put_(
    {pluss::Slicing::Ellipsis(), std::int64_t{2}},
    pluss::make_series({12.5, 22.5, 32.5}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1000, 1001, 12.5, 1003],
  [200, 70, 22.5, 90],
  [3000, 3001, 32.5, 3003]]
```

Example 8. Non-scalar sources must match the selected element count exactly.

```cpp
try {
  grid.extract_put_(
      {std::int64_t{0}, pluss::Slicing::SliceRange(0, 4, 2)},
      pluss::make_series({1.0, 2.0, 3.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::extract_put_: source must be scalar or match selected element count
```

Example 9. Invalid slice step values fail before any write happens.

```cpp
try {
  grid.extract_put_(
      {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(0, 4, 0)},
      pluss::make_series(0.0));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::extract: step cannot be 0
```

## Gotchas And Failure Modes

- `NewAxis()` is accepted, but it does not create a new writable destination
  axis.
- Reverse slices are legal here, so source values follow selector visit order,
  not sorted position order.
- Bool masks and integer index selectors are allowed because this method is
  willing to materialize explicit destination positions.
- A zero slice step fails during selector normalization before any write occurs.
- The selection count, not the visible destination shape, is what governs
  whether a non-scalar source is valid.

## Teaching Summary

Reach for `extract_put_()` when you want the richest selector-language write
surface on `Series`. It is the right method for masks, reverse slices,
`Ellipsis()`, and advanced integer selectors. If you later realize your
selector could have stayed a plain view all along, then
[`traverse_put_()`](traverse_put_.md) is the stricter sibling.

## See Also

- [`Series::extract_put`](extract_put.md): copy-returning sibling.
- [`Series::traverse_put_`](traverse_put_.md): stricter in-place selector
  writes.
- [`Series::extract`](extract.md): read-side counterpart.

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
