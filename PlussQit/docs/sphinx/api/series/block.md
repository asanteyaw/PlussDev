<!-- pluss-hand-authored -->

# Series::block

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::block(std::int64_t row_start,
              std::int64_t row_count,
              std::int64_t col_start,
              std::int64_t col_count) const -> Series
```

## What It Does

Extracts one rectangular 2D sub-block as a view.

`block()` is the matrix-specialized counterpart to nested basic slices. It is
conceptually equivalent to:

```cpp
x.slice(0, row_start, row_start + row_count)
 .slice(1, col_start, col_start + col_count)
```

but it states the intent more directly when the operation is really "give me a
row/column rectangle from this matrix."

## Parameters

**row_start** (`std::int64_t`)

The first row to include. Negative values count from the end.

**row_count** (`std::int64_t`)

How many rows to keep from `row_start`. Must be `>= 0`.

**col_start** (`std::int64_t`)

The first column to include. Negative values count from the end.

**col_count** (`std::int64_t`)

How many columns to keep from `col_start`. Must be `>= 0`.

## Bounds Behavior

`block()` does not clamp or shrink the request for you. After negative-start
normalization:

- the start coordinates must still be in range
- `row_start + row_count` must stay inside dim `0`
- `col_start + col_count` must stay inside dim `1`

Zero-sized rectangles are allowed as long as the start coordinate itself is
valid.

## Returns

`Series`
    A 2D view over the requested rectangle.

## Gradient

`block()` is differentiable. Reverse-mode sensitivity flows only to the selected
rectangle and stays zero everywhere else in the parent matrix.

## Examples

Example 1. Shared setup plus a center-right `2 x 3` rectangle.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});

std::cout << grid.block(1, 2, 1, 3);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[21, 22, 23],
  [31, 32, 33]]
```

The view keeps the parent row stride of `4`, which is why it is non-contiguous
even though the visible rectangle itself is dense.

Example 2. Negative starts are normalized from the end before the rectangle is
checked.

```cpp
std::cout << grid.block(-2, 2, -3, 2);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[21, 22],
  [31, 32]]
```

Example 3. Zero-sized rectangles are valid if the start itself is valid.

```cpp
std::cout << grid.block(3, 0, 1, 2);
```

### Output

```text
pluss::Series(shape=[0,2], strides=[4,1], dtype=float64, platform=cpu, numel=0, contiguous=false)
[]
```

That is not an error. It is an empty 2D view with zero rows and two columns.

Example 4. `block()` is strictly 2D.

```cpp
auto vec = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4});

try {
  std::cout << vec.block(0, 1, 0, 1) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
block: expected a 2D Series
```

Example 5. Backpropagation only marks the selected rectangle.

```cpp
auto tracked_grid = pluss::make_series(
    std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
    {3, 4});
tracked_grid.tracks_slope_(true);

tracked_grid.block(1, 2, 1, 3).sum().reverse();
std::cout << tracked_grid.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 0],
  [0, 1, 1, 1],
  [0, 1, 1, 1]]
```

Only the `2 x 3` selected rectangle receives gradient.

## Notes

- `block()` is 2D-only by design.
- It is often the clearest API when the caller thinks in matrix rectangles,
  rather than in one-axis slices chained together.
- The result usually inherits the parent row stride and is therefore frequently
  non-contiguous.

## See Also

- [`Series::slice`](slice.md): one-axis basic slicing.
- [`Series::traverse_slice`](traverse_slice.md): dynamic view-only slicing.
- [`Series::extract`](extract.md): richer mixed-selector indexing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="traverse_put.html">
    <span class="label">Previous</span>
    <span class="title">traverse_put</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
