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

Returns a cheap strided view over one axis.

This is the basic-slicing helper: one dimension at a time, positive step only,
no advanced index tensors, no reversed traversal, and no selector grammar beyond
`dim/start/end/step`. In exchange, the result stays in the aliasing view lane
instead of materializing gathered data.

That is the right mental split:

- `slice()` is the small, explicit, view-oriented primitive
- [`extract()`](extract.md) is the richer dynamic indexing engine

## Parameters

**dim** (`std::int64_t`)

The axis to slice. Negative dimensions are allowed, so `-1` means the last
axis.

**start** (`std::int64_t`)

The first included index on `dim`. Negative values resolve from the end.

In the one-argument overload `slice(dim, start)`, slicing runs from `start` to
the natural end of the axis.

**end** (`std::int64_t`)

The exclusive stop bound. Negative values resolve from the end.

In `slice(dim)` and `slice(dim, start)`, `end` defaults to the natural end of
the chosen axis.

**step** (`std::int64_t`)

Stride along the sliced axis. This helper requires `step > 0`.

`step == 0` is invalid, and `step < 0` is rejected intentionally because
reversed traversal belongs on the more general [`extract()`](extract.md) surface
rather than this cheap view helper.

## Returns

`Series`
    A strided view over the requested range.

## Gradient

`slice()` is differentiable. During backpropagation, sensitivity flows only to
the selected positions, and skipped positions receive zero.

For stepped slices, this means the backward result is sparse in exactly the same
pattern as the forward selection.

## Examples

Example 1. Shared setup for the overload family.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

std::cout << grid.slice(1, 1, 4, 2);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

This is the full `(dim, start, end, step)` form. The output is non-contiguous
because stepping by `2` skips through the parent storage.

Example 2. Omitting `step` defaults it to `1`.

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

Example 3. Omitting `end` means "slice from `start` to the natural end."

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

Example 4. `slice(dim)` is the explicit no-op slice for that axis.

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

Example 5. Negative dimensions are normalized from the back.

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

Example 6. Negative-step reversal is rejected here.

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

Example 7. Backpropagation only marks the sliced positions.

```cpp
auto tracked_grid = pluss::make_series(
    std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
    {3, 4});
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

Columns `1` and `3` were selected from every row, so those are the only
positions that receive gradient.

## Notes

- `slice()` is for positive-step basic slicing only.
- The result aliases the original storage.
- Even `step == 1` slices are often non-contiguous if they expose only part of a
  larger parent row stride.

## See Also

- [`Series::traverse_slice`](traverse_slice.md): the dynamic view-only selector
  counterpart.
- [`Series::extract`](extract.md): the richer surface for reversed or advanced
  indexing.
- [`Series::block`](block.md): a 2D row/column rectangle helper built from the
  same view semantics.

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
