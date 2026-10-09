<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::inverse

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::inverse() const
Series SeriesLinearAlgProxy::inverse() const
```

## What It Does

`inverse()` computes the multiplicative inverse of each square matrix. If
`B = A.inverse()`, then

```text
A B = I    and    B A = I
```

up to floating-point rounding. The result has the same shape as the input.
Every matrix in a batch is inverted independently.

## Undoing A Linear Transformation

A matrix represents a linear transformation. Its inverse undoes that
transformation: directions stretched by `A` are contracted by `A^-1`, and
directions mixed by `A` are separated again.

An inverse exists only when the transformation preserves every independent
direction. If rows or columns are dependent, information has been collapsed
and no matrix can reconstruct it. That is why singular input is an error for
`inverse()`, while [`det()`](det.md) can simply return zero.

In numerical code, forming an inverse is appropriate when the inverse itself
is the desired result or will be reused as a matrix. If the actual goal is to
solve a linear system, a solve method states that goal more directly and can
avoid materializing the full inverse.

## Shape Contract

| Input shape | Interpretation | Output shape |
| --- | --- | --- |
| `[n, n]` | one square matrix | `[n, n]` |
| `[b, n, n]` | `b` independent matrices | `[b, n, n]` |
| `[..., n, n]` | matrices indexed by leading dimensions | `[..., n, n]` |

The final two dimensions must form square matrices. Leading dimensions are
preserved exactly; there is no cross-batch interaction.

## Returns

`Series`
    The inverse matrix or batch of inverse matrices, with the same shape as
    the input and floating-point values.

## Gradient

If `Y = A^-1` and the incoming matrix sensitivity is `G = dL/dY`, reverse mode
uses

```text
dL/dA = -A^-T G A^-T
```

The negative sign expresses an important fact: increasing `A` in a direction
that strengthens the forward transformation generally weakens its inverse.
For batched matrices, the formula is evaluated independently at every batch
coordinate.

## Examples

Example 1. Invert a two-by-two matrix.

```cpp
auto matrix = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
std::cout << matrix.linearalg.inverse() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.6, -0.7],
  [-0.2, 0.4]]
```

Example 2. Multiplying by the inverse reconstructs the identity. Tiny residuals
around zero are ordinary floating-point rounding.

```cpp
auto matrix = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
auto inverse = matrix.linearalg.inverse();

std::cout << matrix.linearalg.mm(inverse) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, -1.110223e-16],
  [1.110223e-16, 1]]
```

Example 3. Batch dimensions are retained and each matrix receives its own
inverse.

```cpp
auto batch = pluss::make_series({
    {{2.0, 0.0}, {0.0, 4.0}},
    {{1.0, 1.0}, {0.0, 2.0}},
});

std::cout << batch.linearalg.inverse() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[0.5, -0],
  [0, 0.25]],
  [[1, -0.5],
  [0, 0.5]]]
```

Example 4. The gradient depends on both sides of the incoming sensitivity.
Summing the inverse makes `G` a matrix of ones.

```cpp
auto matrix = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
matrix.tracks_slope_(true);

matrix.linearalg.inverse().sum().reverse();

std::cout << matrix.sensitivity() << "\n";
```

Because `A^-T = diag(1/2, 1/4)`, the outer scaling of the all-ones sensitivity
produces the four negative entries below.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-0.25, -0.125],
  [-0.125, -0.0625]]
```

Example 5. A singular matrix cannot be inverted.

```cpp
try {
  auto singular = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  std::cout << singular.linearalg.inverse() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
subsp::math::linalg::inverse: LU factorization failed
```

## Gotchas And Failure Modes

- Input must have at least two dimensions, and its final two dimensions must
  be square.
- Every matrix in a batch must be invertible. One singular member makes the
  batched operation fail.
- Near-singular matrices may produce very large inverse entries and magnify
  floating-point error even when factorization succeeds.
- Identity checks should use a tolerance; exact zero residuals are not
  guaranteed by floating-point arithmetic.

## Summary

`inverse()` returns the matrix that undoes each input transformation. It
preserves all dimensions, supports independent matrix batches, and rejects
singular matrices whose lost directions cannot be recovered.

## See Also

- [`Series::LinearAlgAccessor::det`](det.md): detect signed volume collapse.
- [`Series::LinearAlgAccessor::mm`](mm.md): verify or apply an inverse.
- [`Series::LinearAlgAccessor::slogdet`](slogdet.md): stable determinant sign and magnitude.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dot.html">
    <span class="label">Previous</span>
    <span class="title">dot</span>
  </a>
  <a class="next-link" href="logdet.html">
    <span class="label">Next</span>
    <span class="title">logdet</span>
  </a>
</div>
