<!-- pluss-hand-authored -->

# Series::traverse_slice

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_slice(pluss::RangeView<ExtractSpec> specs) const -> Series
```

## What It Does

`traverse_slice()` builds a dynamic selector view, but only for selectors that
remain view-representable.

That restriction is the whole point of the method. It accepts the same broad
selector language family used by [`extract()`](extract.md), but only the subset
that can still be lowered into ordinary view operations such as scalar select,
basic slice, axis insertion, and ellipsis expansion.

So it sits between two neighboring surfaces:

- more expressive than [`slice()`](slice.md), because it accepts a dynamic
  selector list
- stricter than [`extract()`](extract.md), because it refuses selectors that
  would require materialized gather behavior

If your selector must stay in the aliasing view lane, `traverse_slice()` is the
public surface for that job.

## Parameters

**specs** (`pluss::RangeView<ExtractSpec>`)

The dynamic selector list.

In practical use, you can think of the supported selector pieces like this:

- scalar index: consumes one source axis and removes it from the result rank
- positive-step slice: keeps the axis but may produce a non-contiguous view
- `SliceAll()`: preserves the axis unchanged
- `NewAxis()`: inserts a length-1 axis into the result
- `Ellipsis()`: expands to as many identity slices as needed

The important question is not just "is this selector legal?" but also "can this
selector still be represented as a view?"

## Accepted vs Rejected Selectors

Accepted:

- scalar indices
- positive-step slices
- `SliceAll()`
- `NewAxis()`
- `Ellipsis()`, once normalized

Rejected:

- advanced integer index tensors
- boolean index tensors
- reverse slices
- any selector path that would require materializing gathered data

Those rejected cases belong on [`extract()`](extract.md), not on this
view-preserving surface.

## Returns

`Series`
    A view onto the original storage.

## Gradient

`traverse_slice()` is differentiable for the same reason `slice()` and
`select()` are: it composes only view-representable operations.

Backward therefore routes sensitivity back to exactly the original storage
positions selected by the normalized spec list.

## Examples

The examples below build from simple mixed selectors to axis insertion and
ellipsis behavior, then finish with the important rejection cases and gradient
mapping.

Example 1. A scalar index followed by a stepped slice removes one axis and keeps
one view axis.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});

std::cout << grid.traverse_slice({
    std::int64_t{1},
    pluss::Slicing::SliceRange(1, 4, 2)});
```

### Output

```text
pluss::Series(shape=[2], strides=[2], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[21, 23]
```

The scalar row index removes the row axis. The stepped column slice leaves one
remaining axis with stride `2`.

Example 2. `NewAxis()` inserts a real length-1 axis into the result.

```cpp
std::cout << grid.traverse_slice({
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceAll(),
    std::int64_t{2}});
```

### Output

```text
pluss::Series(shape=[1,3], strides=[12,4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[12, 22, 32]]
```

That inserted axis is not cosmetic. It changes the result rank and can be
useful when later broadcasting logic expects a retained singleton dimension.

Example 3. `Ellipsis()` expands to however many identity slices are needed.

```cpp
std::cout << grid.traverse_slice({
    pluss::Slicing::Ellipsis(),
    std::int64_t{2}});
```

### Output

```text
pluss::Series(shape=[3], strides=[4], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[12, 22, 32]
```

On this rank-2 input, `Ellipsis(), 2` normalizes to the same logical effect as
"keep all earlier axes, then take column `2`."

Example 4. A scalar index can be combined with `NewAxis()` and a positive-step
slice in the same selector.

```cpp
std::cout << grid.traverse_slice({
    std::int64_t{2},
    pluss::Slicing::NewAxis(),
    pluss::Slicing::SliceRange(0, 3, 1)});
```

### Output

```text
pluss::Series(shape=[1,3], strides=[4,1], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[30, 31, 32]]
```

This example is useful because it shows how rank can both shrink and grow
inside one normalized selector path.

Example 5. Advanced index tensors are rejected because they are not
view-representable.

```cpp
try {
  auto row_mask = pluss::make_series({true, false, true}, {3});

  std::cout << grid.traverse_slice({
      row_mask,
      pluss::Slicing::SliceAll()}) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_slice: selector is not view-representable; use extract() or extract_put_() for materializing paths
```

This is one of the key points on the page: the method is limited by
design, not by accident.

Example 6. Reverse slices are rejected for the same reason.

```cpp
try {
  std::cout << grid.traverse_slice({
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)}) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse_slice: selector is not view-representable; use extract() or extract_put_() for materializing paths
```

Negative-step traversal is a materializing-style problem, so it belongs on
`extract()`, not here.

Example 7. Backward marks exactly the storage positions selected by the dynamic
view selector.

```cpp
auto tracked_grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
tracked_grid.tracks_slope_(true);

tracked_grid.traverse_slice({
    std::int64_t{1},
    pluss::Slicing::SliceRange(1, 4, 2)}).sum().reverse();
std::cout << tracked_grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [0, 1, 0, 1],
  [0, 0, 0, 0]]
```

Only row `1`, columns `1` and `3` were selected, so only those positions
receive gradient.

## Gotchas And Failure Modes

- `traverse_slice()` is a view builder, not a general gather engine.
- Scalar indices shrink rank.
- `NewAxis()` grows rank.
- `Ellipsis()` is normalized, not magical; it expands to ordinary identity
  slices.
- Advanced tensors and reverse slices are rejected deliberately.

## Summary

Use `traverse_slice()` when you need a dynamic selector list but still want to
guarantee that the result stays in the cheap view lane.

- Use scalar indices to collapse axes.
- Use positive-step slices to keep axes while narrowing them.
- Use `NewAxis()` when downstream shape alignment needs a singleton axis.
- Use `Ellipsis()` when you want to say "all remaining axes" explicitly.
- Switch to [`extract()`](extract.md) as soon as the selector needs advanced or
  reverse behavior.

The core habit is simple: ask whether the selector can still be represented as
an aliasing view. If yes, `traverse_slice()` is the right surface.

## See Also

- [`Series::slice`](slice.md): fixed-shape basic slicing.
- [`Series::extract`](extract.md): full dynamic selector surface, including
  materializing paths.
- [`Series::traverse_put_`](traverse_put_.md): in-place writes through the same
  normalized view-only selector subset.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="tril.html">
    <span class="label">Next</span>
    <span class="title">tril</span>
  </a>
</div>
