<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::mm

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::mm(const Series& other) const
Series SeriesLinearAlgProxy::mm() const
Series Series::mm(const Series& other) const
```

## What It Does

`mm()` performs one matrix-matrix multiplication and requires both operands to
be two-dimensional. If the left matrix has shape `[m, k]` and the right matrix
has shape `[k, n]`, the result has shape `[m, n]`:

```text
out[i, j] = sum(left[i, t] * right[t, j], over t)
```

The shared `k` dimension disappears because it is summed. The left row count
`m` and right column count `n` survive. Rectangular matrices are therefore
normal: `mm()` does not mean "square matrix multiplication."

## Rows Against Columns

Read the operation as a table of row-column inner products. Every output row
comes from one row of the left matrix; every output column comes from one
column of the right matrix. The entry at `[i, j]` answers: "how strongly does
left row `i` align with right column `j`?"

Use `mm()` rather than [`matmul()`](matmul.md) when rank 2 is part of the
program's contract. A vector or batch then fails immediately instead of
silently selecting a different rank rule.

## Parameter

**other** (`const Series&`)
    The right-hand matrix. It must have rank 2, use the same dtype and platform
    as the left matrix, and satisfy `left.size(1) == other.size(0)`.

## Returns

`Series`
    A new rank-2 `Series` with shape `[left.size(0), other.size(1)]`. Neither
    operand is modified.

## Gradient

For `C = A B` and incoming sensitivity `G = dL/dC`, reverse mode applies:

```text
dL/dA = G B^T
dL/dB = A^T G
```

The transpose turns output-space sensitivity back toward each input's shape.
For complex matrices, the corresponding adjoint uses conjugate transposes.

## Examples

Example 1. The accessor accepts either a method argument or a peer bound when
the proxy is created. Both forms keep `left` as the left operand.

```cpp
auto left = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto right = pluss::make_series({{2.0, 0.0}, {1.0, 2.0}});

std::cout << "peer_argument=" << left.linearalg.mm(right) << "\n";
std::cout << "peer_bound=" << left.linearalg(right).mm() << "\n";
```

The first output entry is the first left row dotted with the first right
column: `1*2 + 2*1 = 4`.

### Output

```text
peer_argument=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [10, 8]]
peer_bound=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [10, 8]]
```

Example 2. Rectangular dimensions determine the result shape. A `[2, 3]`
matrix multiplied by a `[3, 2]` matrix returns `[2, 2]`.

```cpp
auto left = pluss::make_series({
    {1.0, 0.0, 2.0},
    {-1.0, 3.0, 1.0},
});
auto right = pluss::make_series({
    {3.0, 1.0},
    {1.0, 2.0},
    {2.0, 0.0},
});

std::cout << left.linearalg.mm(right) << "\n";
```

For the second row and second column, `(-1)*1 + 3*2 + 1*0 = 5`.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[7, 1],
  [2, 5]]
```

Example 3. Track both matrices through a scalar objective.

```cpp
auto left = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto right = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
left.tracks_slope_(true);
right.tracks_slope_(true);

left.linearalg.mm(right).sum().reverse();

std::cout << "dL_dleft=" << left.sensitivity() << "\n";
std::cout << "dL_dright=" << right.sensitivity() << "\n";
```

Summing the output supplies an all-ones incoming matrix. Each left row gets
the right matrix's row sums `[11, 15]`; each right column gets the left
matrix's column sums `[4, 6]`.

### Output

```text
dL_dleft=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[11, 15],
  [11, 15]]
dL_dright=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [6, 6]]
```

Example 4 (contract check). A vector on either side is not accepted, even if
its length would fit an equivalent `matmul()` expression.

```cpp
try {
  auto matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  std::cout << matrix.linearalg.mm(pluss::make_series({1.0, 2.0})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
mm: expected two 2D Series
```

## Gotchas And Failure Modes

- Both operands must have rank 2; `mm()` never broadcasts batches.
- `left.size(1)` must equal `other.size(0)`.
- Dtypes and platforms must match.
- Matrix multiplication is order-sensitive: `A.mm(B)` and `B.mm(A)` need not
  have the same shape or values.
- Use `mul()` for position-by-position multiplication.

## Summary

`mm()` is the contract-first matrix product. It contracts columns of the left
matrix against rows of the right and always returns a rank-2 matrix.

## See Also

- [`Series::LinearAlgAccessor::matmul`](matmul.md): vectors, matrices, and
  broadcast batches.
- [`Series::LinearAlgAccessor::mv`](mv.md): strict matrix-vector multiplication.
- [`Series::mul`](../series/mul.md): elementwise multiplication.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matrix_power.html">
    <span class="label">Previous</span>
    <span class="title">matrix_power</span>
  </a>
  <a class="next-link" href="mv.html">
    <span class="label">Next</span>
    <span class="title">mv</span>
  </a>
</div>
