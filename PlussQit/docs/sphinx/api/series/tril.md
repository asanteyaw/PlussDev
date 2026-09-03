<!-- pluss-hand-authored -->

# Series::tril

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tril() const -> Series
Series::tril(std::int64_t diagonal) const -> Series
```

## What It Does

`tril()` keeps the lower-triangular region of the trailing matrix dimensions
and zeroes everything above the chosen diagonal.

For a plain 2D matrix, that is the familiar lower triangle. For higher-rank
input, the method applies the same rule batch-wise to the last two axes. All
earlier axes are treated as batch axes.

This is the right mental model:

- rank 2: one matrix
- rank 3+: a stack of matrices
- rank 0 or 1: invalid

## Overload Map

**`tril()`**

Equivalent to `tril(0)`, which keeps the main diagonal and everything below it.

**`tril(std::int64_t diagonal)`**

Shifts the keep boundary.

- positive values keep one or more diagonals above the main diagonal too
- negative values remove part of the main lower triangle

## Parameters

**diagonal** (`std::int64_t`)

The highest diagonal that remains non-zero.

## Returns

`Series`
    A new `Series` with the same shape and dtype as the input, but with the
    disallowed upper-triangular positions replaced by zero.

## Gradient

`tril()` preserves reverse-mode connectivity on the current verified surface.

The backward rule is the same mask as the forward rule:

- positions kept by `tril()` receive sensitivity
- positions zeroed by `tril()` do not

## Examples

Example 1. The default form keeps the main diagonal and everything below it.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.tril();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [4, 5, 0],
  [7, 8, 9]]
```

Example 2. A positive offset keeps one more diagonal above the main diagonal.

```cpp
std::cout << square.tril(1);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 0],
  [4, 5, 6],
  [7, 8, 9]]
```

Example 3. Higher-rank input is treated batch-wise over the last two axes.

```cpp
std::cout << pluss::make_series({
    {
        {1, 2, 3},
        {4, 5, 6},
    },
    {
        {7, 8, 9},
        {10, 11, 12},
    },
}, pluss::ScalarType::Int64).tril();
```

### Output

```text
pluss::Series(shape=[2,2,3], strides=[6,3,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=[[[1, 0, 0],
  [4, 5, 0]],
  [[7, 0, 0],
  [10, 11, 0]]]
```

Example 4. Rank-1 input fails because there is no trailing matrix pair.

```cpp
try {
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).tril() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
tril/triu: expected tensor with at least 2 dimensions
```

Example 5. Reverse-mode keeps sensitivity only in the lower-triangular region
that survived the forward mask.

```cpp
auto tril_grad = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
tril_grad.tracks_slope_(true);
tril_grad.tril().sum().reverse();

std::cout << tril_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [1, 1, 0],
  [1, 1, 1]]
```

## Gotchas And Failure Modes

- `tril()` operates on the last two axes, not necessarily the first two.
- Rank-3+ input is processed batch-wise over trailing matrices.
- Rank-0 and rank-1 inputs are invalid.
- Positive and negative diagonal offsets change the zeroing boundary.
- Reverse-mode uses the same lower-triangular mask as the forward pass.

## Summary

Use `tril()` for lower-triangular masking on matrices or batches of matrices.
It is shape-preserving and value-masking, and reverse-mode follows the same
keep-mask on the current verified surface.

## See Also

- [`Series::triu`](triu.md): upper-triangular companion.
- [`Series::tril_`](tril_.md): in-place lower-triangular masking.
- [`Series::diag`](diag.md): diagonal-only extract-or-embed surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="traverse_slice.html">
    <span class="label">Previous</span>
    <span class="title">traverse_slice</span>
  </a>
  <a class="next-link" href="tril_.html">
    <span class="label">Next</span>
    <span class="title">tril_</span>
  </a>
</div>
