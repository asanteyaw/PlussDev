<!-- pluss-hand-authored -->

# Series::traverse_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put(pluss::RangeView<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a patched copy after writing through a selector that must remain
view-representable.

This is the value-semantic sibling of [`traverse_put_()`](traverse_put_.md).
The write rules are the same, the accepted selector subset is the same, and the
selector normalization is the same. The difference is ownership:

- `traverse_put_()` mutates the current `Series`
- `traverse_put()` leaves the current `Series` alone and returns a modified
  copy

So `traverse_put()` is the method to choose when you want a clean "derive a
patched result" style, but you also want the selector itself constrained to
plain traversal and view semantics.

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The destination selector.

This method accepts only the view-representable subset shared with
[`traverse_slice()`](traverse_slice.md) and [`traverse_put_()`](traverse_put_.md):

- scalar indices
- positive-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`

Rejected selectors include:

- negative-step slices
- bool masks
- integer index `Series`
- any mixed selector that requires materialized advanced indexing

`NewAxis()` is allowed because it is only result-shape bookkeeping; it does not
turn the write into advanced indexing.

**values** (`const Series&`)

The replacement data.

Two value modes are supported:

- scalar source: broadcast across the selected destination region
- non-scalar source: element count must exactly match the selected destination
  element count

The selector gate is stricter than [`extract_put()`](extract_put.md), but the
write-cardinality rules are the same once the selector has been accepted.

## Returns

`Series`
    A new `Series` containing the write result.

## Examples

Example 1. Shared setup. This starting grid matches the state produced by the
verified in-place examples.

```cpp
auto grid = pluss::make_series({
    {1.0, 100.0, 12.0, 13.5},
    {2.0, 200.0, 22.0, 23.5},
    {3.0, 300.0, -5.0, 33.5},
});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13.5],
  [2, 200, 22, 23.5],
  [3, 300, -5, 33.5]]
```

Example 2. A copy-returning write patches the result and leaves the source
unchanged.

```cpp
auto copied = grid.traverse_put(
    {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
    pluss::make_series(999.0));

std::cout << copied;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[999, 999, 12, 13.5],
  [2, 200, 22, 23.5],
  [3, 300, -5, 33.5]]
```

Example 3. The original input is still unchanged after `traverse_put()`.

```cpp
std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13.5],
  [2, 200, 22, 23.5],
  [3, 300, -5, 33.5]]
```

Example 4. `NewAxis()` is still legal on the copy-returning surface.

```cpp
std::cout << grid.traverse_put(
                 {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
                 pluss::make_series({10.0, 20.0, 30.0}));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13.5],
  [20, 200, 22, 23.5],
  [30, 300, -5, 33.5]]
```

The added axis affects selector normalization, not the final writable rank.

Example 5. `Ellipsis()` is part of the accepted view-representable subset here
too.

```cpp
std::cout << grid.traverse_put(
                 {pluss::Slicing::Ellipsis(), std::int64_t{2}},
                 pluss::make_series({120.0, 220.0, 320.0}));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 120, 13.5],
  [2, 200, 220, 23.5],
  [3, 300, 320, 33.5]]
```

Example 6. Reverse slices are rejected because they are not view-representable
on this surface.

```cpp
try {
  std::cout << grid.traverse_put(
                   {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)},
                   pluss::make_series(0.0));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_put: selector is not view-representable; use extract() or extract_put() for materializing paths
```

Example 7. Bool masks are rejected for the same reason.

```cpp
try {
  auto row_mask = pluss::make_series({true, false, true});
  std::cout << grid.traverse_put(
                   {row_mask, pluss::Slicing::SliceAll()},
                   pluss::make_series(0.0));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_put: selector is not view-representable; use extract() or extract_put() for materializing paths
```

Example 8. Cardinality mismatches still fail after selector admission, and that
message comes from the shared selector-write machinery.

```cpp
try {
  std::cout << grid.traverse_put(
                   {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
                   pluss::make_series({1.0, 2.0, 3.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::extract_put: source must be scalar or match selected element count
```

## Gotchas And Failure Modes

- `traverse_put()` is strict by design. It rejects selectors that would require
  materialized advanced indexing.
- `NewAxis()` is accepted, but it is only selector bookkeeping.
- Reverse slices, masks, and integer index `Series` are all rejected here even
  though [`extract_put()`](extract_put.md) can accept them.
- Some later validation errors mention `extract_put` because both methods share
  the same downstream write-cardinality checks after the selector has been
  accepted.

## Summary

Use `traverse_put()` when you want value semantics and a selector gate that
stays inside ordinary traversal and view rules. If your selector needs masked
or materialized addressing, move to [`extract_put()`](extract_put.md). If the
selector is still strict-view-friendly but you want mutation, move to
[`traverse_put_()`](traverse_put_.md).

## See Also

- [`Series::traverse_put_`](traverse_put_.md): in-place sibling.
- [`Series::extract_put`](extract_put.md): copy-returning writes without the
  view-only restriction.
- [`Series::traverse_slice`](traverse_slice.md): read-side companion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="traverse_put_.html">
    <span class="label">Previous</span>
    <span class="title">traverse_put_</span>
  </a>
  <a class="next-link" href="block.html">
    <span class="label">Next</span>
    <span class="title">block</span>
  </a>
</div>
