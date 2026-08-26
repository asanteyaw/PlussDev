<!-- pluss-hand-authored -->

# Series::extract_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::extract_put(pluss::RangeView<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a patched copy after writing through the full selector language.

This is the value-semantic sibling of [`extract_put_()`](extract_put_.md). The
selector grammar is the same, the write cardinality rules are the same, and the
logical write order is the same. The difference is ownership:

- `extract_put_()` mutates the current `Series`
- `extract_put()` leaves the current `Series` unchanged and returns a new one

That makes `extract_put()` the right surface when the write belongs inside an
expression, a pipeline, or any place where "derive a modified result" is
clearer than "mutate this object first."

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The destination selector.

This method accepts the same selector family as [`extract()`](extract.md):

- scalar indices
- slices, including negative-step slices
- `SliceAll()`
- `Ellipsis()`
- `NewAxis()`
- bool masks
- integer index `Series`

Write-side interpretation is the load-bearing point:

- scalar indices narrow the addressed region
- slices preserve an axis and choose positions on it
- `Ellipsis()` expands to the needed identity slices
- `NewAxis()` is accepted for selector compatibility, but does not create a new
  writable destination dimension
- advanced selectors are materialized into explicit destination positions

**values** (`const Series&`)

The source values to write into the selected destinations.

Two value modes are supported:

- scalar source: broadcast across the entire selection
- non-scalar source: `values.numel()` must exactly match the number of selected
  destination elements

As on the in-place sibling, matching is by selected element count rather than
by preserving the visible shape of `values`.

## Returns

`Series`
    A new `Series` containing the write result.

## Examples

Example 1. Shared setup. Start from a grid whose state is easy to track.

```cpp
auto copy_grid = pluss::make_series({
    {100.0, -1.0, 12.0, 13.0},
    {200.0, 70.0, 22.0, 90.0},
    {300.0, -1.0, 32.0, 33.0},
});

std::cout << copy_grid;
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
auto patched = copy_grid.extract_put(
    {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(2, 4, 1)},
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

Example 3. The original input is unchanged after the copy-returning write.

```cpp
std::cout << copy_grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 70, 22, 90],
  [300, -1, 32, 33]]
```

Example 4. Bool masks can patch only the rows that satisfy the mask.

```cpp
auto row_mask = pluss::make_series({true, false, true});

std::cout << copy_grid.extract_put(
                 {row_mask, pluss::Slicing::SliceAll()},
                 pluss::make_series(0.0));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [200, 70, 22, 90],
  [0, 0, 0, 0]]
```

Example 5. Reverse slices are also legal on the copy-returning surface.

```cpp
std::cout << copy_grid.extract_put(
                 {std::int64_t{1}, pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)},
                 pluss::make_series({900.0, 700.0}));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[100, -1, 12, 13],
  [200, 700, 22, 900],
  [300, -1, 32, 33]]
```

The selector visits row `1`, columns `3` then `1`, and the source values are
consumed in that same order.

Example 6. Integer index `Series` selectors can patch explicit rows by copy.

```cpp
auto picked_rows = pluss::make_series({2LL, 0LL});

std::cout << copy_grid.extract_put(
    {picked_rows, pluss::Slicing::SliceAll()},
    pluss::make_series({
        {9000.0, 9001.0, 9002.0, 9003.0},
        {7000.0, 7001.0, 7002.0, 7003.0},
    }));
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[7000, 7001, 7002, 7003],
  [200, 70, 22, 90],
  [9000, 9001, 9002, 9003]]
```

Example 7. Invalid slice steps fail before the copy can be produced.

```cpp
try {
  std::cout << copy_grid.extract_put(
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

- `extract_put()` is copy-returning. The original object stays unchanged.
- `NewAxis()` is accepted as part of selector normalization, but it does not
  create a new writable destination axis.
- Reverse slices, masks, and integer index selectors are all legal because this
  method is willing to materialize destination positions.
- Cardinality is checked against the number of selected destination elements,
  not against the visible destination layout.

## Teaching Summary

Use `extract_put()` when you want the richest selector-language write surface
but also want value semantics. If the selector needs masks, reverse traversal,
or integer advanced selectors, this is the broadest copy-returning method in
the family.

## See Also

- [`Series::extract_put_`](extract_put_.md): in-place sibling.
- [`Series::traverse_put`](traverse_put.md): copy-returning writes with a
  stricter selector gate.
- [`Series::extract`](extract.md): read-side counterpart.

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
