<!-- pluss-hand-authored -->

# Series::diagonal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::diagonal() const -> Series
Series::diagonal(std::int64_t diagonal) const -> Series
Series::diagonal(std::int64_t diagonal, std::int64_t dim1, std::int64_t dim2) const -> Series
```

## What It Does

`diagonal()` always extracts. Unlike [`diag()`](diag.md), it never switches
into matrix-building mode.

That makes it the better root method when you want your call site to state
"read a diagonal" unambiguously, regardless of what rank the input happens to
be.

The current verified root surface is narrower than the name might suggest:

- it is currently implemented for 2D inputs
- the three-argument overload accepts only the two real matrix axes
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

The first matrix axis.

**dim2** (`std::int64_t`)

The second matrix axis.

Both dimension parameters accept negative indexing, but they must refer to two
different axes after normalization.

On the current verified root surface, the practical choices for a 2D matrix are
just:

- `(0, 1)`
- `(1, 0)`

The first reads the diagonal in the ordinary orientation. The second reads the
diagonal after the matrix axes have been conceptually swapped.

## Returns

`Series`
    A 1D `Series` containing the extracted diagonal values.

## Gradient

`diagonal()` shares the same verified extraction route as matrix-extract
[`diag()`](diag.md).

That means reverse-mode sensitivity is scattered back onto the selected
diagonal positions and leaves every non-participating position untouched.

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

## Gotchas And Failure Modes

- `diagonal()` always extracts. If you wanted a diagonal matrix, use
  [`diag_embed()`](diag_embed.md) or [`diagflat()`](diagflat.md) instead.
- The current root surface is 2D-only.
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
