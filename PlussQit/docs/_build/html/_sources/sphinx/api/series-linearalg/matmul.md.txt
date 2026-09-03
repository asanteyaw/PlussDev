<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::matmul

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::matmul(const Series& other) const
Series SeriesLinearAlgProxy::matmul() const
Series Series::matmul(const Series& other) const
```

## What It Does

`matmul()` multiplies vectors, matrices, or batches of matrices. Unlike an
elementwise product, it contracts one dimension: entries along the shared
inner dimension are multiplied and summed.

For two matrices with shapes `[m, k]` and `[k, n]`, the result has shape
`[m, n]`. Each output entry is

```text
out[i, j] = sum(left[i, t] * right[t, j], over t)
```

The same idea extends to vectors and batches. Rank determines which dimensions
are contracted and which dimensions survive in the result.

## Contracting Algebraic Dimensions

Start from the matrix rule, then treat vectors as temporary one-row or
one-column matrices:

- a left vector behaves like shape `[1, k]`; its temporary row dimension is
  removed from the result
- a right vector behaves like shape `[k, 1]`; its temporary column dimension
  is removed from the result
- two vectors lose both temporary dimensions and therefore return a scalar
- higher-rank inputs keep their leading dimensions as broadcastable batches

This promotion-and-removal model predicts all five rank patterns without
memorizing unrelated special cases.

## Shape Rules

| Left shape | Right shape | Result shape | Meaning |
| --- | --- | --- | --- |
| `[k]` | `[k]` | `[]` | vector inner product |
| `[k]` | `[k, n]` | `[n]` | row vector by matrix |
| `[m, k]` | `[k]` | `[m]` | matrix by column vector |
| `[m, k]` | `[k, n]` | `[m, n]` | matrix by matrix |
| `[..., m, k]` | `[..., k, n]` | `[..., m, n]` | batched matrix product |

For batched products, only the leading batch dimensions broadcast. The last
two dimensions remain matrix dimensions and must satisfy the inner-dimension
rule.

For example, `[1, 2, 2]` multiplied by `[5, 2, 3]` produces `[5, 2, 3]`:
the batch dimension `1` expands to `5`, while every individual multiplication
still has the form `[2, 2]` by `[2, 3]`.

## Parameter

**other** (`const Series&`)
    The right operand.

    `other` must be at least one-dimensional, use the same dtype and platform
    as the left operand, and have a compatible contracted dimension. Leading
    batch dimensions must be broadcastable when either operand has rank above
    two.

## Returns

`Series`
    A new `Series` containing the contracted product. Neither input is
    modified.

    Vector-vector multiplication returns a rank-zero scalar. Matrix and
    batched cases return the shapes listed above.

## Gradient

`matmul()` participates in reverse-mode tracking for floating and complex
inputs.

For a matrix product `C = A B` and incoming sensitivity `G = dL/dC`, the two
matrix sensitivities are

```text
dL/dA = G B^T
dL/dB = A^T G
```

For complex inputs, the corresponding adjoints use conjugate transposes.

When a batch dimension was broadcast in the forward pass, its reverse
contributions are summed back into the original input shape. A matrix reused
across five batches therefore receives the sum of all five batch
contributions, not a five-batch sensitivity with the wrong shape.

## Examples

Example 1. Multiply a 2x3 matrix by a 3x2 matrix.

```cpp
auto left = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
auto right = pluss::make_series({
    {7.0, 8.0},
    {9.0, 10.0},
    {11.0, 12.0},
});

std::cout << left.linearalg.matmul(right) << "\n";
```

The top-left output is `1*7 + 2*9 + 3*11 = 58`. The other three entries use
the same row-by-column rule.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[58, 64],
  [139, 154]]
```

Example 2. Rank changes the output shape in predictable ways.

```cpp
auto vector = pluss::make_series({1.0, 2.0, 3.0});
auto matrix = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
});

std::cout << "vector_vector="
          << vector.linearalg(pluss::make_series({4.0, 5.0, 6.0})).matmul()
          << "\n";
std::cout << "vector_matrix=" << vector.linearalg.matmul(matrix) << "\n";
std::cout << "matrix_vector="
          << matrix.transpose(0, 1).linearalg.matmul(vector) << "\n";
```

The vector-vector case contracts its only dimension and therefore has no
dimension left: its shape is `[]`. Both mixed-rank cases leave one dimension
and return vectors.

### Output

```text
vector_vector=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=32
vector_matrix=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[22, 28]
matrix_vector=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[22, 28]
```

Example 3. Leading batch dimensions broadcast before each matrix product.

```cpp
auto left = pluss::make_series({
    {
        {1.0, 2.0},
        {3.0, 4.0},
    },
});
auto right = pluss::make_series({
    {
        {1.0, 0.0},
        {0.0, 1.0},
    },
    {
        {2.0, 0.0},
        {0.0, 3.0},
    },
});

std::cout << left.linearalg.matmul(right) << "\n";
```

`left` has one matrix batch and `right` has two. The left matrix is reused for
both products. Multiplication by the first right matrix preserves it;
multiplication by the second scales its first and second columns by `2` and
`3` respectively.

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 2],
  [3, 4]],
  [[2, 6],
  [6, 12]]]
```

Example 4. Both operands can receive reverse sensitivity.

```cpp
auto left = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});
auto right = pluss::make_series({
    {5.0, 6.0},
    {7.0, 8.0},
});
left.tracks_slope_(true);
right.tracks_slope_(true);

left.linearalg.matmul(right).sum().reverse();

std::cout << "dL_dleft=" << left.sensitivity() << "\n";
std::cout << "dL_dright=" << right.sensitivity() << "\n";
```

Because the objective sums every output, its incoming sensitivity is a 2x2
matrix of ones. Multiplying that by `right.transpose(0, 1)` gives the repeated
row `[11, 15]`. The right sensitivity follows from
`left.transpose(0, 1)` multiplied by the same ones matrix.

### Output

```text
dL_dleft=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[11, 15],
  [11, 15]]
dL_dright=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [6, 6]]
```

Example 5. A mismatched inner dimension is rejected.

```cpp
try {
  std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.matmul(
      pluss::make_series({{1.0, 2.0}})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

Both operands have shape `[1, 2]`. The left inner dimension is `2`, but the
right product dimension is `1`, so no matrix product is defined.

### Output

```text
matmul: shape mismatch
```

Example 6. `matmul()` does not silently promote mismatched dtypes.

```cpp
try {
  auto floats = pluss::make_series({{1.0, 2.0}});
  auto integers = pluss::make_series(
      {{1, 2}, {3, 4}}, pluss::ScalarType::Int64);
  std::cout << floats.linearalg.matmul(integers) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
matmul: dtype mismatch
```

## Gotchas And Failure Modes

- Both operands must have rank at least one. Scalar-by-Series multiplication
  belongs to the elementwise arithmetic family.
- The contracted dimensions must match exactly.
- Batch dimensions broadcast; matrix dimensions do not.
- The inputs must use the same dtype. Convert deliberately before multiplying
  when mixed numeric types are intentional.
- The inputs must be on the same platform.
- `matmul()` is out-of-place and never mutates either operand.

## Summary

`matmul()` is the rank-polymorphic product. Read the last one or two dimensions
as vector or matrix dimensions, broadcast only the leading dimensions, and
expect the shared inner dimension to disappear through summation.

## See Also

- [`SeriesLinearAlgAccessor::mm`](mm.md): strict matrix-matrix product.
- [`SeriesLinearAlgAccessor::mv`](mv.md): strict matrix-vector product.
- [`SeriesLinearAlgAccessor::dot`](dot.md): inner product of two vectors.
- [`Series::mul`](../series/mul.md): elementwise multiplication.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lu_solve.html">
    <span class="label">Previous</span>
    <span class="title">lu_solve</span>
  </a>
  <a class="next-link" href="matrix_exp.html">
    <span class="label">Next</span>
    <span class="title">matrix_exp</span>
  </a>
</div>
