<!-- pluss-hand-authored -->

# Series::traverse_put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put_(pluss::RangeView<ExtractSpec> specs, const Series& values) -> Series&
```

## What It Does

Writes into the current `Series` through a selector that must stay
view-representable, mutates in place, and returns the same handle.

This is the write-side companion to [`traverse_slice()`](traverse_slice.md). It
accepts dynamic selector syntax, but only the subset that can still be lowered
into ordinary view operations such as selecting, slicing, and inserting a
logical axis marker.

That restriction is not an accident. It is the reason to choose the method at
all. `traverse_put_()` gives you selector-based writes while still guarding
against drifting into masked or gathered writes that would need materialized
destination positions.

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The destination selector.

Accepted selector forms are the same view-representable subset used by
[`traverse_slice()`](traverse_slice.md):

- scalar indices
- positive-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`

Rejected selector forms include:

- negative-step slices
- bool masks
- integer index `Series`
- any mixed selector that requires advanced materialization

`NewAxis()` is still legal because it is shape bookkeeping, not a new writable
destination dimension.

**values** (`const Series&`)

The source values to write into the selected destination region.

Two value modes are accepted:

- scalar source: broadcast across every selected destination element
- non-scalar source: element count must exactly match the selected destination
  element count

This is a stricter selector gate than [`extract_put_()`](extract_put_.md), not
a different write-cardinality model.

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

Example 2. A whole-column write is accepted because the destination remains a
plain view.

```cpp
grid.traverse_put_(
    {pluss::Slicing::SliceAll(), std::int64_t{1}},
    pluss::make_series({100.0, 200.0, 300.0}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, 32, 33]]
```

Example 3. `NewAxis()` is accepted here too. It participates in selector
normalization, but it does not create a new writable destination dimension.

```cpp
grid.traverse_put_(
    {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
    pluss::make_series({1.0, 2.0, 3.0}));

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
grid.traverse_put_(
    {std::int64_t{2}, pluss::Slicing::SliceRange(2, 4, 1)},
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

Example 5. `Ellipsis()` is also part of the accepted view-representable subset.

```cpp
grid.traverse_put_(
    {pluss::Slicing::Ellipsis(), std::int64_t{3}},
    pluss::make_series({13.5, 23.5, 33.5}));

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13.5],
  [2, 200, 22, 23.5],
  [3, 300, -5, 33.5]]
```

Example 6. Reverse slices are rejected because they are not view-representable
on this stricter surface.

```cpp
try {
  grid.traverse_put_(
      {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)},
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

Example 7. Bool masks are rejected for the same reason.

```cpp
try {
  auto row_mask = pluss::make_series({true, false, true});
  grid.traverse_put_(
      {row_mask, pluss::Slicing::SliceAll()},
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

Example 8. Cardinality mismatches still fail, and the message bubbles from the
shared selector-write machinery.

```cpp
try {
  grid.traverse_put_(
      {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
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

## Gotchas And Failure Modes

- `traverse_put_()` is strict by design. If a selector would require
  materialized advanced indexing, this method rejects it.
- `NewAxis()` is accepted, but it is only a normalization marker, not a new
  writable dimension.
- Reverse slices, bool masks, and integer index `Series` are all disallowed
  here even though [`extract_put_()`](extract_put_.md) can handle them.
- Some validation errors still mention `extract_put_()` because both methods
  share the same underlying write-cardinality checks after selector admission.

## Teaching Summary

Choose `traverse_put_()` when you want selector-language writes but you also
want the API to enforce "stay in ordinary view territory." It is the strict
in-place sibling of [`traverse_slice()`](traverse_slice.md). The moment your
selector needs masks, reverse traversal, or advanced integer selection, switch
to [`extract_put_()`](extract_put_.md).

## See Also

- [`Series::traverse_put`](traverse_put.md): copy-returning sibling.
- [`Series::traverse_slice`](traverse_slice.md): read-side companion.
- [`Series::extract_put_`](extract_put_.md): broader in-place selector writes.

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
