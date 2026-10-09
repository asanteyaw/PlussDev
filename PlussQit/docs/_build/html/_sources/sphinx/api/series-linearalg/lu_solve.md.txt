<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::lu_solve

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::lu_solve(const Series& b) const
Series SeriesLinearAlgProxy::lu_solve() const
```

## What It Does

Solves `A X = B` for a general nonsingular square coefficient matrix `A`, which
is the `Series` owning the accessor. Despite the method's name, callers pass
the original coefficient matrix; factorization and pivoting happen inside the
operation. Do not pass a packed LU factor.

## General Systems Through Elimination

LU elimination decomposes a general square system into triangular stages while
pivoting protects against unsuitable diagonal choices. It handles matrices
that need not be symmetric or positive definite. When stronger structure is
known, a Cholesky or triangular solve can avoid unnecessary work.

## Parameter And Shapes

`b` may be `[...,n]` or `[...,n,k]`. Vector right-hand sides return
`[...,n,1]`; matrix right-hand sides preserve `[...,n,k]`. Leading dimensions
of `A` and `b` broadcast, while `A` must end in `[n,n]`.

## Gradient

For `X=A^-1 B` and incoming `G`, reverse mode uses
`dB=A^-T G` and `dA=-(A^-T G)X^T`. Broadcasted batch sensitivities reduce back
to each original operand shape.

## Examples

Example 1. Solve one right-hand side.

```cpp
auto a = pluss::make_series({{3.0, 1.0}, {1.0, 2.0}});
std::cout << a.linearalg.lu_solve(pluss::make_series({9.0, 8.0})) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[2],
  [3]]
```

Example 2. Columns of a matrix right-hand side are solved together.

```cpp
auto a = pluss::make_series({{3.0, 1.0}, {1.0, 2.0}});
auto b = pluss::make_series({{9.0, 1.0}, {8.0, 0.0}});
std::cout << a.linearalg.lu_solve(b) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 0.4],
  [3, -0.2]]
```

## Gotchas And Failure Modes

- The owner is the original coefficient matrix, not a precomputed LU object.
- Singular matrices fail because the solution is not unique.
- A very ill-conditioned matrix may produce a large residual even when formally nonsingular.
- Prefer solving to explicitly multiplying by `inverse()`.

## Summary

`lu_solve()` solves general square systems, supports multiple and broadcasted
right-hand sides, and differentiates with respect to both coefficients and data.

## See Also

- [`cholesky_solve`](cholesky_solve.md): SPD systems from a reusable factor.
- [`triangular_solve`](triangular_solve.md): systems already in triangular form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logdet.html"><span class="label">Previous</span><span class="title">logdet</span></a>
  <a class="next-link" href="matmul.html"><span class="label">Next</span><span class="title">matmul</span></a>
</div>
