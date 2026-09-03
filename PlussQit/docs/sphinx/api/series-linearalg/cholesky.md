<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::cholesky

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::cholesky(bool upper = false) const
Series SeriesLinearAlgProxy::cholesky(bool upper = false) const
```

## What It Does

`cholesky()` factors each symmetric positive-definite matrix into one triangular
factor. With the default lower form it returns `L` such that `A = L L^T`. With
`upper=true` it returns `U` such that `A = U^T U`.

## Factoring Positive Energy

A positive-definite matrix assigns strictly positive quadratic energy
`x^T A x` to every nonzero vector. Cholesky turns that coupled energy into a
triangular coordinate transform. The factor is unique when its diagonal is
positive, making it a particularly efficient foundation for solves and
inverses involving covariance and normal-equation matrices.

## Parameter

**upper** (`bool`, default `false`)
    Return the upper factor when `true`; otherwise return the lower factor.
    This changes which triangle contains meaningful values, not the shape.

## Shape Contract

Input `[...,n,n]` produces `[...,n,n]`. The last two dimensions must be square;
every leading batch item is factored independently.

## Gradient

Reverse mode differentiates the factorization identity and returns a symmetric
sensitivity for the original matrix. The input must remain positive definite;
the derivative becomes poorly conditioned as an eigenvalue approaches zero.

## Examples

Example 1. Reconstruct the matrix from the default lower factor.

```cpp
auto a = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
auto l = a.linearalg.cholesky();
std::cout << l << "\n";
std::cout << l.linearalg.mm(l.transpose(0, 1)) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 0],
  [1, 1.4142136]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 2],
  [2, 3]]
```

Example 2. `upper=true` returns the transposed orientation.

```cpp
auto a = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
std::cout << a.linearalg.cholesky(true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 1],
  [0, 1.4142136]]
```

Example 3. A non-positive-definite matrix is rejected rather than producing an
invalid factor.

```cpp
try {
  std::cout << pluss::make_series({{1.0, 2.0}, {2.0, 1.0}}).linearalg.cholesky();
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
subsp::math::linalg::cholesky: matrix is not SPD
```

## Gotchas And Failure Modes

- Symmetry alone is insufficient; every eigenvalue must be strictly positive.
- Only the selected triangle represents the factor; the opposite triangle is zero.
- Near-singular positive-definite matrices can yield unstable factors and gradients.

## Summary

`cholesky()` converts an SPD matrix into a lower or upper triangular factor,
preserving batches and supporting reverse mode.

## See Also

- [`cholesky_solve`](cholesky_solve.md): solve from an existing factor.
- [`cholesky_inverse`](cholesky_inverse.md): invert from an existing factor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_linearalg_accessor_api_reference.html"><span class="label">Previous</span><span class="title">SeriesLinearAlgAccessor</span></a>
  <a class="next-link" href="cholesky_inverse.html"><span class="label">Next</span><span class="title">cholesky_inverse</span></a>
</div>
