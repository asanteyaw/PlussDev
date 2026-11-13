<!-- pluss-hand-authored -->

# Series::diagonal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::diagonal() const
Series Series::diagonal(std::int64_t diagonal) const
Series Series::diagonal(std::int64_t diagonal, std::int64_t dim1, std::int64_t dim2) const
```

## What It Does

`diagonal()` always extracts. Unlike [`diag()`](diag.md), it never switches
into matrix-building mode.

That makes it the better root method when you want your call site to state
"read a diagonal" unambiguously, regardless of what rank the input happens to
be.

It works on any input with at least two axes:

- the three-argument overload names which two axes form the "matrix" the
  diagonal is read from; every other axis is carried along unchanged
- the extracted diagonal becomes the **last** axis of the result
- swapping `dim1` and `dim2` changes which direction the diagonal is read from

## Overload Map

**`diagonal()`**

Extract the main diagonal.

**`diagonal(std::int64_t diagonal)`**

Extract an offset diagonal.

**`diagonal(std::int64_t diagonal, std::int64_t dim1, std::int64_t dim2)`**

Extract an offset diagonal while naming the two participating axes explicitly.

## Parameters

**diagonal** (`std::int64_t`)

The diagonal offset.

- `0` selects the main diagonal
- positive values move toward later columns
- negative values move toward later rows

**dim1** (`std::int64_t`)

The first of the two axes the diagonal is read from.

**dim2** (`std::int64_t`)

The second of the two axes the diagonal is read from.

Both dimension parameters accept negative indexing, but they must refer to two
different axes after normalization.

For a 2D matrix the choices are `(0, 1)` (the ordinary orientation) and
`(1, 0)` (the diagonal after the two axes have been swapped). For higher-rank
input any two distinct axes may be named; the offset is measured from `dim1`
toward `dim2`.

## Returns

`Series`
    For a 2D input, a 1D `Series` of the diagonal. For higher rank, the input
    shape with `dim1` and `dim2` removed and the diagonal's length appended as
    the last axis.

## Gradient

Reverse-mode sensitivity is scattered back onto the selected diagonal
positions and leaves every non-participating position untouched, for any rank.

## Examples

Example 1. The zero-argument form extracts the main diagonal.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.diagonal();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 5, 9]
```

Example 2. A positive offset reads above the main diagonal.

```cpp
auto rectangular = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

std::cout << rectangular.diagonal(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 6]
```

Example 3. Swapping `dim1` and `dim2` changes which matrix orientation is used
before the diagonal is read.

```cpp
auto rectangular = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

std::cout << rectangular.diagonal(0, 1, 0);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 5]
```

That result comes from reading the diagonal of the transposed orientation, not
from the ordinary `(0, 1)` orientation.

Example 4. The named axes must differ.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

try {
  std::cout << square.diagonal(0, 0, 0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
diagonal: dim1 and dim2 must differ
```

Example 5. Higher-rank input: name the two axes, and the diagonal becomes the
last axis.

```cpp
auto cube = pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}, {2, 3, 4});

// the diagonal of each 3 x 4 slice, one row per slice
std::cout << cube.diagonal(0, 1, 2);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 6, 11],
  [13, 18, 23]]
```

Axis 0 is carried through, and the length-3 diagonal of every slice is the new
last axis.

Example 6. The offset and the axes combine; the offset is measured from `dim1`
toward `dim2`.

```cpp
auto cube = pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}, {2, 3, 4});

std::cout << cube.diagonal(1, 1, 2) << "\n";
std::cout << cube.diagonal(0, 0, 2);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[2, 7, 12],
  [14, 19, 24]]
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 14],
  [5, 18],
  [9, 22]]
```

## Gotchas And Failure Modes

- `diagonal()` always extracts. If you wanted a diagonal matrix, use
  [`diag_embed()`](diag_embed.md) or [`diagflat()`](diagflat.md) instead.
- Inputs need at least two axes; a 1D or scalar `Series` throws.
- Swapping `dim1` and `dim2` is not cosmetic. It changes the extraction
  orientation.
- Reverse-mode follows the same diagonal-scatter rule as matrix-extract
  [`diag()`](diag.md).

## Summary

Use `diagonal()` when you want extraction-only semantics with no ambiguity
about embedding. It is the clearer choice whenever input rank should not change
the meaning of the call.

## See Also

- [`Series::diag`](diag.md): dual-mode extract-or-embed surface.
- [`Series::trace`](trace.md): diagonal extraction followed by reduction.
- [`Series::diagonal_scatter`](diagonal_scatter.md): copy-returning diagonal
  write surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diagflat.html">
    <span class="label">Previous</span>
    <span class="title">diagflat</span>
  </a>
  <a class="next-link" href="diagonal_scatter.html">
    <span class="label">Next</span>
    <span class="title">diagonal_scatter</span>
  </a>
</div>
