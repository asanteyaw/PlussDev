<!-- pluss-hand-authored -->

# Series::traverse_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put(pluss::RangeView<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a patched copy of the series after writing through a selector that must
remain view-representable.

This is the value-semantic sibling of [`traverse_put_()`](traverse_put_.md).
The write rules are the same, the accepted selector subset is the same, and the
selector normalization is the same. The only difference is ownership:

- `traverse_put_()` mutates the current series
- `traverse_put()` leaves the current series alone and returns a modified copy

So `traverse_put()` is the method to choose when you want a clean "derive a
patched result" style, but you also want the selector itself constrained to
plain traversal/view semantics.

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
- integer advanced index series
- any mixed selector that requires materialized advanced indexing

`NewAxis()` is allowed because it is only result-shape bookkeeping; it does not
turn the write into advanced indexing.

**values** (`const Series&`)

The replacement data.

Two value modes are supported:

- scalar source: broadcast across the selected destination region
- non-scalar source: element count must exactly match the selected destination
  element count

## Returns

`Series`
    A new series containing the write result.

## Examples

Example 1. Shared setup. This starting grid matches the state produced by the
in-place `traverse_put_()` examples.

```cpp
auto grid = pluss::make_series(
    {1.0, 100.0, 12.0, 13.0,
     2.0, 200.0, 22.0, 23.0,
     3.0, 300.0, -5.0, -5.0},
    {3, 4});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13],
  [2, 200, 22, 23],
  [3, 300, -5, -5]]
```

Example 2. A copy-returning write patches the result and leaves the source
unchanged.

```cpp
const auto first_two = std::vector<ExtractSpec>{
    std::int64_t{0},
    pluss::Slicing::SliceRange(0, 2, 1)};

auto copied = grid.traverse_put(
    first_two,
    pluss::make_series(999.0));

std::cout << copied;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[999, 999, 12, 13],
  [2, 200, 22, 23],
  [3, 300, -5, -5]]
```

Example 3. The original input is still unchanged after `traverse_put()`.

```cpp
std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 100, 12, 13],
  [2, 200, 22, 23],
  [3, 300, -5, -5]]
```

Example 4. `NewAxis()` is still legal on the copy-returning surface.

```cpp
const auto with_newaxis = std::vector<ExtractSpec>{
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{0}};

std::cout << grid.traverse_put(
    with_newaxis,
    pluss::make_series({10.0, 20.0, 30.0}, {3}));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, -5, -5]]
```

The added axis affects selector normalization, not the final writable rank.

Example 5. Reverse slices are rejected because they are not view-representable
on this surface.

```cpp
try {
  const auto reverse_specs = std::vector<ExtractSpec>{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};

  std::cout << grid.traverse_put(
      reverse_specs,
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

Example 6. Bool masks are rejected for the same reason.

```cpp
try {
  auto row_mask = pluss::make_series({true, false, true}, {3});
  const auto mask_specs = std::vector<ExtractSpec>{
      row_mask,
      pluss::Slicing::SliceAll()};

  std::cout << grid.traverse_put(
      mask_specs,
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

## Notes

- `traverse_put()` is the strict, copy-returning selector-write surface.
- Choose it when the selector should stay in the ordinary traversal/view lane
  and mutation would be undesirable.
- If the selector needs masks, reverse steps, or explicit advanced indexing,
  use [`extract_put()`](extract_put.md) instead.
- If you want the same selector gate but in place, use
  [`traverse_put_()`](traverse_put_.md).

## See Also

- [`Series::traverse_put_`](traverse_put_.md): in-place sibling with the same
  selector gate.
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
