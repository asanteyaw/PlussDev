<!-- pluss-hand-authored -->

# Series::slice

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::slice(std::int64_t dim,
              std::int64_t start,
              std::int64_t end,
              std::int64_t step = 1) const -> Series

Series::slice(std::int64_t dim, std::int64_t start, std::int64_t end) const -> Series
Series::slice(std::int64_t dim, std::int64_t start) const -> Series
Series::slice(std::int64_t dim) const -> Series
```

## What It Does

`slice()` returns a cheap strided view over one axis.

This is the small, explicit, basic-slicing helper. It is meant for view-style
reads where the indexing grammar is simple:

- one axis at a time
- positive-step traversal only
- no advanced index tensors
- no reverse traversal

That tradeoff is the point of the method. Because `slice()` stays in the
view-representable lane, it can alias the original storage instead of
materializing gathered data.

The right mental split is:

- `slice()` is the compact, explicit, view-first primitive
- [`extract()`](extract.md) is the richer indexing engine when the selector is
  more sophisticated

## Parameters

**dim** (`std::int64_t`)

The axis to slice.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

**start** (`std::int64_t`)

The first included index on `dim`.

- negative values resolve from the end of that axis
- in `slice(dim, start)`, slicing continues from `start` to the natural end

**end** (`std::int64_t`)

The exclusive stop bound.

- negative values resolve from the end of the axis
- in `slice(dim)` and `slice(dim, start)`, `end` defaults to the natural end

**step** (`std::int64_t`)

Stride along the sliced axis.

- `step > 0` is required
- `step == 1` means contiguous progression along that axis
- `step > 1` skips positions and usually makes the result non-contiguous
- `step == 0` is invalid
- `step < 0` is rejected intentionally on this surface

Reverse traversal belongs on the more general [`extract()`](extract.md) path,
not on this cheap view helper.

## Returns

`Series`
    A strided view over the requested range.

## Gradient

`slice()` is differentiable.

In backpropagation, sensitivity flows only to the selected positions, and
skipped positions receive zero. For stepped slices, the backward result is
sparse in exactly the same pattern as the forward selection.

## Examples

The examples below build from the full four-argument form toward the shorter
overloads, then cover negative dims, failure modes, and gradient behavior.

Example 1. The full `(dim, start, end, step)` form slices one axis with an
explicit stride.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});

std::cout << grid.slice(1, 1, 4, 2);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

The output is non-contiguous because stepping by `2` skips through the parent
storage.

Example 2. Changing the sliced axis changes both the shape and the traversal
meaning.

```cpp
std::cout << grid.slice(0, 1, 3);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[20, 21, 22, 23],
  [30, 31, 32, 33]]
```

Here `dim = 0`, so whole rows are being kept instead of selected columns.

Example 3. Omitting `step` defaults it to `1`.

```cpp
std::cout << grid.slice(1, 2, 4);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[12, 13],
  [22, 23],
  [32, 33]]
```

Example 4. Omitting `end` means "slice from `start` to the natural end."

```cpp
std::cout << grid.slice(1, 1);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[4,1], dtype=float64, platform=cpu, numel=9, contiguous=false)
data=[[11, 12, 13],
  [21, 22, 23],
  [31, 32, 33]]
```

Example 5. `slice(dim)` is the explicit identity slice for that axis.

```cpp
std::cout << grid.slice(1);
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 11, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 33]]
```

This is useful when code wants to stay in a slicing pipeline but one branch
happens to keep the full axis.

Example 6. Negative dims are normalized from the back.

```cpp
std::cout << grid.slice(-1, 0, 4, 2);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[10, 12],
  [20, 22],
  [30, 32]]
```

For a rank-2 `Series`, `-1` means the last axis, so this is column slicing.

Example 7. Negative-step reversal is intentionally rejected here.

```cpp
try {
  std::cout << grid.slice(1, -1, pluss::Slicing::Slice::kUnbounded, -1) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::slice: negative-step slices are not supported by this cheap view helper
```

This is not an accident. Reverse traversal belongs on a richer indexing
surface.

Example 8. Zero step is also illegal.

```cpp
try {
  std::cout << grid.slice(1, 0, 4, 0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::slice: step must be non-zero
```

Example 9. Backpropagation marks exactly the positions that were sliced.

```cpp
auto tracked_grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
tracked_grid.tracks_slope_(true);

tracked_grid.slice(1, 1, 4, 2).sum().reverse();
std::cout << tracked_grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 1, 0, 1],
  [0, 1, 0, 1],
  [0, 1, 0, 1]]
```

Columns `1` and `3` were selected from every row, so only those positions
receive gradient.

## Gotchas And Failure Modes

- `slice()` is for positive-step basic slicing only.
- `step > 1` usually makes the result non-contiguous.
- Even `step == 1` slices can be non-contiguous if they expose only part of a
  larger parent stride.
- Negative dims are fine; negative steps are not.
- `step == 0` is invalid.

## Teaching Summary

Use `slice()` when your selector can be expressed as "one axis, one range, one
positive stride" and you want to stay in the cheap view path.

- Choose the axis with `dim`.
- Use `start` and `end` to express the kept interval.
- Use `step` when regular skipping is part of the meaning.
- Reach for [`extract()`](extract.md) when you need reverse slices or richer
  selector grammar.

If your indexing idea still sounds simple in plain language, `slice()` is often
the right surface.

## See Also

- [`Series::traverse_slice`](traverse_slice.md): dynamic view-only slicing with
  a richer selector list.
- [`Series::extract`](extract.md): full mixed-selector indexing, including
  materializing paths.
- [`Series::block`](block.md): 2D rectangle helper built from the same view
  semantics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pile.html">
    <span class="label">Previous</span>
    <span class="title">pile</span>
  </a>
  <a class="next-link" href="traverse_slice.html">
    <span class="label">Next</span>
    <span class="title">traverse_slice</span>
  </a>
</div>
