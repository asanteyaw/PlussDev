<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::mv

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::mv(const Series& vector) const
Series SeriesLinearAlgProxy::mv() const
Series Series::mv(const Series& vector) const
```

## What It Does

`mv()` multiplies one rank-2 matrix by one rank-1 vector. For a matrix with
shape `[m, k]` and vector with shape `[k]`, the result has shape `[m]`:

```text
out[i] = sum(matrix[i, t] * vector[t], over t)
```

Each matrix row contributes one scalar, so the number of rows becomes the
output length. The column dimension is paired with the vector and contracted.

## Rows As Dot Products

The vector supplies weights for the matrix's columns. Multiplying by
`[1, 0, 0]` selects the first column; multiplying by `[0, 0, 1]` selects
the third. General vectors form weighted combinations of all columns.

This column-combination view and the equivalent row-dot-product view describe
the same computation. Use whichever makes the surrounding algorithm easier to
reason about.

## Parameter

**vector** (`const Series&`)
    The rank-1 right operand. Its length must equal the matrix's column count,
    and its dtype and platform must match the matrix.

## Returns

`Series`
    A new rank-1 `Series` with shape `[matrix.size(0)]`. The matrix and
    vector are unchanged.

## Gradient

For `y = A x` and incoming vector sensitivity `g = dL/dy`:

```text
dL/dA = outer(g, x)
dL/dx = A^T g
```

Every output sensitivity scales one copy of `x` into the corresponding matrix
row. The vector sensitivity gathers contributions from every matrix row.

## Examples

Example 1. Compute one row-vector inner product per matrix row.

```cpp
auto matrix = pluss::make_series({
    {2.0, 0.0, -1.0},
    {1.0, 3.0, 2.0},
});
auto vector = pluss::make_series({4.0, 5.0, 6.0});

std::cout << matrix.linearalg.mv(vector) << "\n";
```

The rows produce `2*4 + 0*5 - 1*6 = 2` and
`1*4 + 3*5 + 2*6 = 31`.

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 31]
```

Example 2. Basis vectors expose the column-combination interpretation.

```cpp
auto matrix = pluss::make_series({
    {2.0, -1.0, 4.0},
    {0.0, 3.0, 5.0},
});

std::cout << "first_column="
          << matrix.linearalg.mv(pluss::make_series({1.0, 0.0, 0.0}))
          << "\n";
std::cout << "third_column="
          << matrix.linearalg.mv(pluss::make_series({0.0, 0.0, 1.0}))
          << "\n";
```

### Output

```text
first_column=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 0]
third_column=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 5]
```

Example 3. Reverse a summed matrix-vector product through both operands.

```cpp
auto matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto vector = pluss::make_series({5.0, 6.0});
matrix.tracks_slope_(true);
vector.tracks_slope_(true);

matrix.linearalg.mv(vector).sum().reverse();

std::cout << "dL_dmatrix=" << matrix.sensitivity() << "\n";
std::cout << "dL_dvector=" << vector.sensitivity() << "\n";
```

The incoming sensitivity is `[1, 1]`. Its outer product with the vector
copies `[5, 6]` into each matrix row. `A^T [1, 1]` sums the matrix columns
to `[4, 6]`.

### Output

```text
dL_dmatrix=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[5, 6],
  [5, 6]]
dL_dvector=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

Example 4 (contract check). Two vectors are a `dot()` case, not an `mv()`
case.

```cpp
try {
  std::cout << pluss::make_series({1.0, 2.0}).linearalg.mv(
      pluss::make_series({3.0, 4.0})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
mv: expected a 2D matrix and a 1D vector
```

## Gotchas And Failure Modes

- The left operand must have rank 2 and the right operand rank 1.
- The matrix column count must equal the vector length.
- `mv()` does not broadcast a vector across matrix batches; use
  [`matmul()`](matmul.md) when batching is intentional.
- Dtypes and platforms must match.
- Operand order is fixed: vector-by-matrix multiplication is a different
  `matmul()` rank pattern.

## Summary

`mv()` is a strict matrix-by-vector operation. It contracts columns against
vector positions and returns one value per matrix row.

## See Also

- [`SeriesLinearAlgAccessor::matmul`](matmul.md): rank-polymorphic products.
- [`SeriesLinearAlgAccessor::mm`](mm.md): strict matrix-matrix products.
- [`SeriesLinearAlgAccessor::dot`](dot.md): strict vector inner products.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mm.html">
    <span class="label">Previous</span>
    <span class="title">mm</span>
  </a>
  <a class="next-link" href="norm.html">
    <span class="label">Next</span>
    <span class="title">norm</span>
  </a>
</div>
