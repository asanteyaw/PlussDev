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

`block()` extracts one rectangular 2D sub-block as a view.

This is the matrix-specialized counterpart to chaining basic slices. It is
conceptually equivalent to taking a row slice and then a column slice, but it
states the intent more directly when the operation is really "give me this
rectangle from the matrix."

That directness matters. When code is thinking in matrix windows, patches,
regions, or rectangular selections, `block()` usually communicates the intent
better than two separate `slice()` calls.

## Parameters

**row_start** (`std::int64_t`)

The first row to include.

- negative values count from the end
- after normalization, the start row must still be valid

**row_count** (`std::int64_t`)

How many rows to keep from `row_start`.

- must be `>= 0`
- the requested rectangle is not clamped for you

**col_start** (`std::int64_t`)

The first column to include.

- negative values count from the end
- after normalization, the start column must still be valid

**col_count** (`std::int64_t`)

How many columns to keep from `col_start`.

- must be `>= 0`
- again, the request is not silently shrunk for you

## Bounds Behavior

`block()` does not clamp or shrink the request.

After negative-start normalization:

- the start coordinates must still be in range
- `row_start + row_count` must stay inside dimension `0`
- `col_start + col_count` must stay inside dimension `1`

Zero-sized rectangles are allowed as long as the start coordinate itself is
valid.

## Returns

`Series`
    A 2D view over the requested rectangle.

## Gradient

`block()` is differentiable.

Reverse-mode sensitivity flows only to the selected rectangle and remains zero
everywhere else in the parent matrix.

## Examples

The examples below build from ordinary rectangular reads to negative starts,
empty rectangles, failure modes, and gradient behavior.

Example 1. `block()` reads a center-right `2 x 3` rectangle as a view.

```cpp
auto grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});

std::cout << grid.block(1, 2, 1, 3);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[21, 22, 23],
  [31, 32, 33]]
```

The result is non-contiguous because it inherits the parent row stride of `4`
instead of being repacked.

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

Example 3. Zero-height rectangles are legal if the start itself is valid.

```cpp
std::cout << grid.block(3, 0, 1, 2);
```

### Output

```text
pluss::Series(shape=[0,2], strides=[4,1], dtype=float64, platform=cpu, numel=0, contiguous=false)
[]
```

That is not an error. It is an empty 2D view with zero rows and two columns.

Example 4. Zero-width rectangles are also legal under the same rule.

```cpp
std::cout << grid.block(1, 2, 4, 0);
```

### Output

```text
pluss::Series(shape=[2,0], strides=[4,1], dtype=float64, platform=cpu, numel=0, contiguous=false)
[]
```

This time the rectangle has rows but no columns.

Example 5. Requests that run out of bounds fail explicitly.

```cpp
try {
  std::cout << grid.block(2, 2, 0, 2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
block: requested rows exceed dim-0 bounds
```

This is an important difference from forgiving slicing APIs in other systems:
`block()` does not silently shrink the request.

Example 6. `block()` is strictly 2D.

```cpp
auto vec = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4});

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

`block()` is not a generic rank-N rectangle helper. It is intentionally a
matrix-only API.

Example 7. Backpropagation only marks the selected rectangle.

```cpp
auto tracked_grid = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
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

Only the selected `2 x 3` rectangle receives gradient.

## Gotchas And Failure Modes

- `block()` is 2D-only by design.
- Negative starts are normalized, but requests are not clamped.
- Zero-sized rectangles are legal if their starts are valid.
- Out-of-bounds rectangles fail instead of silently shrinking.
- The result usually inherits parent stride and is therefore often
  non-contiguous.

## Teaching Summary

Use `block()` when your mental model is genuinely "a rectangle inside a
matrix."

- Choose `row_start` and `col_start` as the rectangle origin.
- Choose `row_count` and `col_count` as its height and width.
- Expect a view, not a repacked copy.
- Expect strict 2D semantics and strict bounds checking.

If your indexing problem sounds like a matrix region instead of a generic slice
expression, `block()` is usually the clearest public surface.

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
