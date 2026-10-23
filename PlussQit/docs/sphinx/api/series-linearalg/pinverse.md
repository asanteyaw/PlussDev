<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::pinverse

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::pinverse(double rcond = 1e-12) const
Series SeriesLinearAlgProxy::pinverse(double rcond = 1e-12) const
```

## What It Does

`pinverse()` computes the Moore-Penrose pseudoinverse of a matrix. It extends
ordinary inversion to rectangular and rank-deficient matrices.

If a matrix has reduced singular-value decomposition

```text
A = U diag(s) V^T
```

then its pseudoinverse is

```text
A+ = V diag(s+) U^T
```

where each retained singular value is replaced by its reciprocal and each
discarded singular value becomes zero. For input shape `[..., m, n]`, the
result has shape `[..., n, m]`.

## Reversing Only Represented Directions

An ordinary inverse must undo a one-to-one square transformation. A
pseudoinverse instead asks for the least-squares reversal that behaves like an
inverse on directions the matrix actually represents.

The singular vectors identify independent input and output directions. Their
singular values measure how strongly those directions are transmitted.
`pinverse()` reciprocates reliable directions and removes directions below the
configured cutoff. This gives a meaningful result when a matrix is tall, wide,
or has collapsed dimensions.

The result is characterized by four Penrose identities:

```text
A A+ A = A
A+ A A+ = A+
(A A+)^T = A A+
(A+ A)^T = A+ A
```

The products `A A+` and `A+ A` are projections onto the represented output and
input subspaces, respectively.

## Parameter

**rcond** (`double`, default `1e-12`)
    Relative singular-value cutoff. A singular value `s[i]` is retained only
    when

    ```text
    s[i] > rcond * largest_singular_value
    ```

    Increasing `rcond` removes more weak directions and reduces the effective
    rank. Zero retains every strictly positive singular value. The value must
    be finite and non-negative.

## Shape Contract

| Input shape | Output shape | Interpretation |
| --- | --- | --- |
| `[m, n]` | `[n, m]` | one pseudoinverse |
| `[b, m, n]` | `[b, n, m]` | one per batch member |
| `[..., m, n]` | `[..., n, m]` | leading dimensions preserved |

Unlike `inverse()`, the final dimensions do not need to be equal. They exchange
positions in the result.

## Returns

`Series`
    The SVD-defined pseudoinverse, with the final two dimensions reversed and
    the input floating or complex dtype preserved. Integer and boolean inputs
    are rejected rather than silently converted.

## Gradient

Reverse mode is defined while the effective rank remains constant. Let
`P = A+` and let `G` be the incoming sensitivity for `P`. For real matrices,

```text
dL/dA = -P^T G P^T
        + (I - A P) G^T P P^T
        + P^T P G^T (I - P A)
```

The first term is the familiar inverse-like contribution. The residual
projection terms account for directions outside the represented row and column
spaces.

At a singular value exactly crossing the `rcond` threshold, effective rank
changes discretely, so the derivative is not continuous. Complex reverse mode
is not currently defined.

## Examples

Example 1. A tall full-column-rank matrix produces a wide pseudoinverse.

```cpp
auto matrix = pluss::make_series({
    {1.0, 0.0},
    {0.0, 1.0},
    {1.0, 1.0},
});

std::cout << matrix.linearalg.pinverse() << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.66666667, -0.33333333, 0.33333333],
  [-0.33333333, 0.66666667, 0.33333333]]
```

Example 2. The first Penrose identity reconstructs the original matrix.

```cpp
auto matrix = pluss::make_series({
    {1.0, 0.0},
    {0.0, 1.0},
    {1.0, 1.0},
});
auto pseudoinverse = matrix.linearalg.pinverse();

std::cout << matrix.linearalg.matmul(pseudoinverse)
                     .linearalg.matmul(matrix) << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 5.5511151e-17],
  [0, 1],
  [1, 1]]
```

The tiny residual is floating-point rounding; mathematically the first entry
of the second column is zero.

Example 3. A wide matrix reverses into a tall pseudoinverse.

```cpp
auto matrix = pluss::make_series({
    {1.0, 0.0, 1.0},
    {0.0, 1.0, 1.0},
});

std::cout << matrix.linearalg.pinverse() << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.66666667, -0.33333333],
  [-0.33333333, 0.66666667],
  [0.33333333, 0.33333333]]
```

Example 4. Rank-deficient square matrices still have a pseudoinverse.

```cpp
auto rank_one = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
std::cout << rank_one.linearalg.pinverse() << "\n";
```

The second row is twice the first, so only one singular direction is retained.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.04, 0.08],
  [0.08, 0.16]]
```

Unlike [`inverse()`](inverse.md), this operation does not fail merely because
the matrix is singular.

Example 5. `rcond` controls which weak directions participate.

```cpp
auto matrix = pluss::make_series({{1.0, 0.0}, {0.0, 0.001}});

std::cout << "default=" << matrix.linearalg.pinverse() << "\n";
std::cout << "rcond_0_01=" << matrix.linearalg.pinverse(0.01) << "\n";
```

With `rcond=0.01`, the cutoff is `0.01 * 1`. The singular value `0.001` falls
below it and is discarded instead of being reciprocated to `1000`.

### Output

```text
default=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 1000]]
rcond_0_01=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 0]]
```

Example 6. Batches preserve their leading dimensions and reverse each matrix
tail.

```cpp
auto batch = pluss::make_series({
    {{1.0, 0.0}, {0.0, 2.0}, {1.0, 1.0}},
    {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
});

std::cout << batch.linearalg.pinverse() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2,3], strides=[6,3,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[[0.55555556, -0.22222222, 0.44444444],
  [-0.11111111, 0.44444444, 0.11111111]],
  [[0.5, 0, 0],
  [0, 0.5, 0.5]]]
```

Example 7. Reverse mode includes both inverse-like and projection terms.

```cpp
auto matrix = pluss::make_series({
    {1.0, 0.0},
    {0.0, 1.0},
    {1.0, 1.0},
});
matrix.tracks_slope_(true);

matrix.linearalg.pinverse().sum().reverse();

std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-0.11111111, -0.11111111],
  [-0.11111111, -0.11111111],
  [-0.55555556, -0.55555556]]
```

Example 8. A negative cutoff is rejected because retaining directions by a
negative relative threshold has no meaningful rank interpretation.

```cpp
try {
  auto matrix = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
  std::cout << matrix.linearalg.pinverse(-0.1) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
subsp::linearalg::pinverse: rcond must be finite and non-negative
```

## Gotchas And Failure Modes

- `pinverse()` swaps the last two dimensions; it does not preserve matrix shape
  for rectangular input.
- `rcond` is relative to the largest singular value of each matrix, not an
  absolute threshold shared across a batch.
- A larger cutoff improves resistance to amplification from weak directions
  but deliberately lowers effective rank.
- Rank changes at the cutoff are discrete. Avoid placing an optimization
  trajectory directly on a threshold when smooth sensitivities are required.
- Input must have at least two dimensions. Each final matrix may be square,
  tall, or wide.

## Summary

`pinverse()` reverses the reliable singular directions of square or rectangular
matrices and removes directions below a relative cutoff. It supports
rank-deficient inputs, swaps matrix dimensions, preserves batches, and obeys
the Moore-Penrose projection identities.

## See Also

- [`Series::LinearAlgAccessor::inverse`](inverse.md): strict inverse for nonsingular square matrices.
- [`Series::LinearAlgAccessor::det`](det.md): detect rank loss in square matrices.
- [`Series::LinearAlgAccessor::matmul`](matmul.md): apply and verify projection identities.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="norm.html">
    <span class="label">Previous</span>
    <span class="title">norm</span>
  </a>
  <a class="next-link" href="qr_q.html">
    <span class="label">Next</span>
    <span class="title">qr_q</span>
  </a>
</div>
