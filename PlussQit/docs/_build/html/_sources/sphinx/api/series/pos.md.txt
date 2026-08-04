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

Returns exactly one element as a rank-0 scalar `Series`.

`pos()` is the narrowest scalar-read surface on `Series`. It does not accept
selector objects, it does not preserve length-1 axes, and it does not try to
flatten mismatched ranks into "something sensible." You choose the overload that
matches the rank you mean, and `pos()` returns the single scalar at that
coordinate.

That narrowness is useful. If the intent is "read one cell," `pos()` states
that directly and enforces it directly.

## Parameters

**index / row / col / i / j / k** (`std::int64_t`)

The scalar coordinate(s) to read.

Each coordinate accepts negative indexing on its own axis:

- `-1` means the last element on that axis
- `-2` means the second from the end

After negative-index normalization, each coordinate is bounds-checked.

## Rank Contract

The overload and the `Series` rank must match exactly:

- `pos(index)` requires a 1D `Series`
- `pos(row, col)` requires a 2D `Series`
- `pos(i, j, k)` requires a 3D `Series`

`pos()` does not implicitly flatten, squeeze, or reinterpret rank.

## Returns

`Series`
    A rank-0 scalar `Series` holding the selected value.

## Gradient

`pos()` is differentiable. In reverse mode, the incoming scalar sensitivity is
scattered back to exactly one source position, and every other source element
gets zero.

That makes `pos()` the scalar-read analogue of [`select()`](index_select.md) and
other view-style selectors: the value path narrows to one cell, and the
backward path widens back out to a sparse mask at that cell.

## Examples

Example 1. Shared setup for the three rank-specific overloads.

```cpp
auto vec = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4});
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
auto cube = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8},
    {2, 2, 2});

std::cout << vec.pos(-1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=40
```

Example 2. The 2D overload normalizes each axis independently.

```cpp
std::cout << mat.pos(1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=6
```

Row `1` means the second row; column `-1` means the last column of that row.

Example 3. The 3D overload follows the same rule on all three axes.

```cpp
std::cout << cube.pos(1, -1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=8
```

Example 4. Rank mismatch is an error, not an implicit flatten.

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

Example 5. Backpropagating through `pos()` produces a one-hot sensitivity map.

```cpp
auto tracked_vec = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4});
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

## Notes

- `pos()` always returns a scalar rank-0 `Series`, never a retained length-1
  axis.
- Use `pos()` when rank is part of the meaning of the call, not just incidental.
- If you need dynamic selector grammar, use [`extract()`](extract.md) or
  [`index()`](index.md) instead.

## See Also

- [`Series::slice`](slice.md): keep a range instead of collapsing to one scalar.
- [`Series::extract`](extract.md): dynamic indexing with richer selector kinds.
- [`Series::block`](block.md): rectangular 2D selection rather than a single
  scalar cell.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
