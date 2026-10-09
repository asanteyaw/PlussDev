<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::cholesky_solve

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::cholesky_solve(const Series& b, bool upper = false) const
Series SeriesLinearAlgProxy::cholesky_solve(bool upper = false) const
```

## What It Does

Solves `A X = B` from an already computed Cholesky factor owned by the
accessor. If the owner is lower `L`, the represented matrix is `A=L L^T`; if it
is upper `U`, `A=U^T U`. Do not pass the original unfactored matrix.

## Reusing A Factorization

The solve performs two triangular substitutions. Computing Cholesky once and
reusing its factor for many right-hand sides avoids repeating the expensive
factorization and preserves the positive-definite structure of the problem.

## Parameters And Shapes

**b** is `[...,n]` or `[...,n,k]`; a vector result is `[...,n,1]` and a matrix
result preserves `[...,n,k]`. Batch dimensions broadcast with the factor.

**upper** selects whether the owner is upper. Its value must match the toggle
used to create the factor.

## Gradient

Reverse mode differentiates both triangular stages. Sensitivities can flow to
the factor and to `b`; the factor sensitivity is restricted to its represented
triangle.

## Examples

Example 1. Factor once, then solve.

```cpp
auto a = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
auto l = a.linearalg.cholesky();
std::cout << l.linearalg.cholesky_solve(pluss::make_series({2.0, 1.0})) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.5],
  [0]]
```

Example 2. An upper factor gives the same solution when `upper=true`.

```cpp
auto a = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
auto u = a.linearalg.cholesky(true);
std::cout << u.linearalg.cholesky_solve(pluss::make_series({2.0, 1.0}), true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.5],
  [0]]
```

## Gotchas And Failure Modes

- The owner is a Cholesky factor, not `A` itself.
- A mismatched `upper` toggle silently describes a different matrix.
- Zero diagonal entries make the represented system singular.
- Prefer this method over forming an inverse when only solutions are needed.

## See Also

- [`cholesky`](cholesky.md): compute the required factor.
- [`cholesky_inverse`](cholesky_inverse.md): materialize the represented inverse.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cholesky_inverse.html"><span class="label">Previous</span><span class="title">cholesky_inverse</span></a>
  <a class="next-link" href="det.html"><span class="label">Next</span><span class="title">det</span></a>
</div>
