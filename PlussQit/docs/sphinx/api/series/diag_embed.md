<!-- pluss-hand-authored -->

# Series::diag_embed

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::diag_embed() const
Series Series::diag_embed(std::int64_t diagonal) const
Series Series::diag_embed(std::int64_t diagonal, std::int64_t dim1, std::int64_t dim2) const
```

## What It Does

`diag_embed()` turns the **last axis** of the input into a diagonal. Every
leading axis is a batch axis, so a `[batch, n]` input becomes `batch` separate
`n x n` diagonal matrices, and a plain vector becomes a single diagonal
matrix.

For an input of shape `[b0, b1, ..., n]` the result holds, for every batch
index, a square plane of side `n + abs(diagonal)` that is zero everywhere
except along the requested diagonal. The plane's two axes are placed at
`dim1` and `dim2` of the result, which by default are the last two.

To build a single diagonal matrix from a multi-axis input by flattening it
first, use [`diagflat()`](diagflat.md).

## Overload Map

**`diag_embed()`**

Main diagonal, plane in the last two axes.

**`diag_embed(std::int64_t diagonal)`**

Shifts the write target: positive values place the diagonal above the main
diagonal, negative values below it.

**`diag_embed(std::int64_t diagonal, std::int64_t dim1, std::int64_t dim2)`**

Offset diagonal and explicit placement of the plane's two axes in the result.

## Parameters

**diagonal** (`std::int64_t`)

The diagonal offset. If the last axis has `n` values, each plane has side
`n + abs(diagonal)`.

**dim1**, **dim2** (`std::int64_t`)

Where the plane's row axis and column axis go in the **result** (which has one
more axis than the input). Negative values count from the end of the result.
They must refer to different axes. The default is `(-2, -1)`. The offset is
measured from `dim1` toward `dim2`, so swapping them mirrors the plane.

## Returns

`Series`
    Rank `input.dim() + 1`: the batch axes in order, with the two plane axes
    inserted at `dim1` and `dim2`.

## Gradient

`diag_embed()` is differentiable. The reverse pass reads each input value back
from the diagonal position it was written to; every other position of the
result contributes nothing.

## Examples

Example 1. A 1D input becomes a square diagonal matrix.

```cpp
std::cout << pluss::make_series({4, 5, 6}, pluss::ScalarType::Int64).diag_embed();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[4, 0, 0],
  [0, 5, 0],
  [0, 0, 6]]
```

Example 2. A positive offset moves the diagonal above the main diagonal and
grows the matrix by `abs(diagonal)`.

```cpp
std::cout << pluss::make_series({4, 5, 6}, pluss::ScalarType::Int64).diag_embed(1);
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=int64, platform=cpu, numel=16, contiguous=true)
data=[[0, 4, 0, 0],
  [0, 0, 5, 0],
  [0, 0, 0, 6],
  [0, 0, 0, 0]]
```

Example 3. Leading axes are batch axes: each row becomes its own matrix.

```cpp
auto rows = pluss::make_series({
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64);

std::cout << rows.diag_embed();
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 0],
  [0, 2]],
  [[3, 0],
  [0, 4]]]
```

The `2 x 2` input becomes a stack of two `2 x 2` diagonal matrices, shape
`[2, 2, 2]`, one per row.

Example 4. `dim1` and `dim2` choose where the plane's axes land in the result.

```cpp
auto rows = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

auto moved = rows.diag_embed(0, 0, 1);
std::cout << moved.sizes()[0] << " " << moved.sizes()[1] << " " << moved.sizes()[2] << "\n";
std::cout << moved;
```

### Output

```text
3 3 2
pluss::Series(shape=[3,3,2], strides=[3,1,9], dtype=int64, platform=cpu, numel=18, contiguous=false)
data=[[[1, 4],
  [0, 0],
  [0, 0]],
  [[0, 0],
  [2, 5],
  [0, 0]],
  [[0, 0],
  [0, 0],
  [3, 6]]]
```

With the plane at axes `(0, 1)`, the batch axis moves to the end: element
`moved[i, j, b]` is the plane entry `(i, j)` of batch `b`.

Example 5. Reverse-mode reads sensitivity back from the embedded diagonal.

```cpp
auto diag_embed_grad = pluss::make_series({1.0, 2.0, 3.0});
diag_embed_grad.tracks_slope_(true);
diag_embed_grad.diag_embed().sum().reverse();

std::cout << diag_embed_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

Example 6. The two plane axes must differ.

```cpp
try {
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).diag_embed(0, 1, 1) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
diag_embed: dim1 and dim2 must differ
```

## Gotchas And Failure Modes

- The **last** axis is the one embedded; every other axis is preserved as a
  batch axis. This differs from [`diagflat()`](diagflat.md), which flattens
  the whole input first.
- Offsets increase each plane's side by `abs(diagonal)`; all non-diagonal
  positions are zero.
- `dim1` and `dim2` index the **result**, which has one more axis than the
  input.
- A scalar (rank-0) input has no axis to embed and fails.

## Summary

`diag_embed()` writes the last axis of the input onto a diagonal of new planes:

- leading axes are batch axes and are preserved
- `diagonal` shifts the diagonal and grows the plane
- `dim1`/`dim2` place the plane in the result
- differentiable; `diagflat()` is the flatten-first alternative

## See Also

- [`Series::diag`](diag.md): dual-mode extract-or-embed surface.
- [`Series::diagflat`](diagflat.md): flatten first, then embed one matrix.
- [`Series::diagonal`](diagonal.md): the inverse, extraction-only surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diag.html">
    <span class="label">Previous</span>
    <span class="title">diag</span>
  </a>
  <a class="next-link" href="diagflat.html">
    <span class="label">Next</span>
    <span class="title">diagflat</span>
  </a>
</div>
