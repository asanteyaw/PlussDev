<!-- pluss-hand-authored -->

# Series::pos

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::pos(std::int64_t index) const -> Series
Series::pos(std::int64_t row, std::int64_t col) const -> Series
Series::pos(std::int64_t i, std::int64_t j, std::int64_t k) const -> Series
```

## What It Does

`pos()` reads exactly one element and returns it as a scalar `Series`.

This is the narrowest coordinate-based read surface on `Series`. It does not
accept selector objects, it does not preserve length-1 axes, and it does not
reinterpret rank for you. You choose the overload that matches the rank you
mean, and `pos()` returns the single value at that coordinate.

That narrowness is useful. If your intent is truly "read one cell," `pos()`
states that intent more clearly than any broader indexing surface.

## Parameters

**index / row / col / i / j / k** (`std::int64_t`)

The coordinate or coordinates to read.

Each coordinate supports negative indexing on its own axis:

- `-1` means the last position on that axis
- `-2` means the second-to-last position

After negative-index normalization, each coordinate is bounds-checked.

## Rank Contract

The overload and the source rank must match exactly:

- `pos(index)` requires a 1D `Series`
- `pos(row, col)` requires a 2D `Series`
- `pos(i, j, k)` requires a 3D `Series`

`pos()` does not implicitly flatten, squeeze, or reinterpret rank. If the rank
does not match the overload, the call fails.

## Returns

`Series`
    A rank-0 scalar `Series` holding the selected value.

## Gradient

`pos()` is differentiable.

In reverse mode, the incoming scalar sensitivity is scattered back to exactly
one source position, and every other element receives zero. So `pos()` is a
single-cell read in the forward pass and a one-hot write-back in the backward
pass.

## Examples

The examples below build from simple scalar reads to rank matching and gradient
behavior.

Example 1. On a 1D `Series`, `pos(-1)` reads the last element.

```cpp
auto vec = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4});

std::cout << vec.pos(-1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=40
```

`pos()` always returns a scalar `Series`, so the shape is `[]`.

Example 2. On a matrix, each axis is normalized independently.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.pos(1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=6
```

Row `1` means the second row. Column `-1` means the last column of that row.

Example 3. Negative indexing also works on earlier axes, not just the last one.

```cpp
std::cout << mat.pos(-1, 0);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

This is the first element of the last row.

Example 4. The 3D overload follows the same per-axis normalization rule.

```cpp
auto cube = pluss::make_series({
    {
        {1.0, 2.0},
        {3.0, 4.0},
    },
    {
        {5.0, 6.0},
        {7.0, 8.0},
    },
});

std::cout << cube.pos(1, -1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=8
```

Example 5. Rank mismatch is an error, not an invitation to flatten or guess.

```cpp
try {
  std::cout << vec.pos(0, 0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::pos: expected 2-D Series for scalar selection
```

This is deliberate. `pos()` only works when the rank contract is explicit and
correct.

Example 6. Backpropagating through `pos()` produces a one-hot sensitivity map.

```cpp
auto tracked_vec = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4});
tracked_vec.tracks_slope_(true);

tracked_vec.pos(-1).reverse();
std::cout << tracked_vec.sensitivity();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 0, 1]
```

Only the selected element receives gradient.

## Gotchas And Failure Modes

- `pos()` always returns a scalar rank-0 `Series`.
- Negative indices are normalized separately on each axis.
- The overload must match the source rank exactly.
- `pos()` does not flatten or reinterpret the source shape for you.

## Summary

Use `pos()` when the meaning of your code is "read one element at this exact
coordinate."

- Use the overload that matches the actual rank.
- Use negative indices when end-relative access is clearer.
- Expect a scalar result every time.

If you need anything more dynamic than a fixed scalar coordinate, move up to a
broader indexing surface such as [`slice()`](slice.md), [`extract()`](extract.md),
or [`index()`](index.md).

## See Also

- [`Series::slice`](slice.md): keep a range instead of collapsing to one scalar.
- [`Series::extract`](extract.md): dynamic mixed-selector indexing.
- [`Series::block`](block.md): rectangular 2D selection instead of a single
  scalar cell.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="polygamma.html">
    <span class="label">Previous</span>
    <span class="title">polygamma</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
