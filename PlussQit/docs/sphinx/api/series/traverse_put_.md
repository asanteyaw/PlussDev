<!-- pluss-hand-authored -->

# Series::traverse_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current series through a selector that must stay
view-representable, mutates in place, and returns `*this`.

This is the write-side companion to [`traverse_slice()`](traverse_slice.md). It
accepts dynamic selector syntax, but only the subset that can still be lowered
into ordinary view operations such as `select()`, `slice()`, and `unsqueeze()`.

That restriction is not a limitation by accident. It is the reason to use the
method at all. `traverse_put_()` gives you selector-based writes while still
guarding against drifting into masked or gathered writes that would need
materialized destination positions.

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

Accepted selector forms are the same view-representable subset used by
[`traverse_slice()`](traverse_slice.md):

- scalar indices
- positive-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`

Rejected selectors include:

- negative-step slices
- bool masks
- integer advanced index series
- any mixed selector that requires materialized advanced indexing

`NewAxis()` is still legal because it is only shape bookkeeping.

**values** (`const Series&`)

The source values to write into the selected destination region.

Two value modes are accepted:

- scalar source: broadcast across every selected destination element
- non-scalar source: element count must exactly match the selected destination
  element count

As on the other write surfaces, dtype and platform are normalized before the
assignment happens.

## Returns

`Series&`
    The same series, after mutation.

## Examples

Example 1. Shared setup. Start from a simple `3 x 4` grid.

```cpp
auto grid = pluss::make_series(
    {10.0, 11.0, 12.0, 13.0,
     20.0, 21.0, 22.0, 23.0,
     30.0, 31.0, 32.0, 33.0},
    {3, 4});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 33]]
```

Example 2. A whole-column write is accepted because the destination is plainly
view-representable.

```cpp
const auto middle_column = std::vector<ExtractSpec>{
    pluss::Slicing::SliceAll(),
    std::int64_t{1}};

grid.traverse_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(middle_column),
    pluss::make_series({100.0, 200.0, 300.0}, {3}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, 32, 33]]
```

Example 3. `NewAxis()` is accepted here too. It does not create a new writable
destination axis; it only participates in selector normalization.

```cpp
const auto with_newaxis = std::vector<ExtractSpec>{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{0}};

grid.traverse_put_(
    pluss::msharp::RangeView::Of<ExtractSpec>(with_newaxis),
    pluss::make_series({1.0, 2.0, 3.0}, {3}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13],
  [2, 200, 22, 23],
  [3, 300, 32, 33]]
```

Example 4. Scalar broadcast still works after the stricter selector gate has
accepted the destination.

```cpp
const auto row_tail = std::vector<ExtractSpec>{
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
data=[[1, 100, 12, 13],
  [2, 200, 22, 23],
  [3, 300, -5, -5]]
```

Example 5. Reverse slices are rejected even though the broader
[`extract_put_()`](extract_put_.md) surface could handle them.

```cpp
try {
  const auto reverse_specs = std::vector<ExtractSpec>{
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

Example 6. Bool masks are rejected for the same reason: they require
materialized destination positions rather than plain traversal.

```cpp
try {
  auto row_mask = pluss::make_series({true, false, true}, {3});
  const auto mask_specs = std::vector<ExtractSpec>{
      row_mask,
      pluss::Slicing::SliceAll()};

  grid.traverse_put_(
      pluss::msharp::RangeView::Of<ExtractSpec>(mask_specs),
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

## Notes

- `traverse_put_()` is the strictest selector-write method on `Series`.
- Use it when selector-based writes should stay in the same ordinary
  traversal/view lane as [`traverse_slice()`](traverse_slice.md).
- If the selector needs masks, reverse steps, or explicit advanced indexing,
  move to [`extract_put_()`](extract_put_.md).
- If you want value semantics instead of mutation, use
  [`traverse_put()`](traverse_put.md).

## See Also

- [`Series::traverse_put`](traverse_put.md): copy-returning sibling with the
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
