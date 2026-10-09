<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::cholesky_inverse

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::cholesky_inverse(bool upper = false) const
Series SeriesLinearAlgProxy::cholesky_inverse(bool upper = false) const
```

## What It Does

Builds `A^-1` from the owning Cholesky factor. Lower mode interprets the owner
as `L` in `A=L L^T`; upper mode interprets it as `U` in `A=U^T U`.

## Inverting The Represented SPD Matrix

The method solves against every column of the identity, sharing one triangular
factor across all columns. The result is symmetric. Use it when the inverse is
itself required; for `A X=B`, `cholesky_solve()` avoids materializing a dense
inverse and is usually the stronger numerical choice.

## Parameter And Shape

`upper` selects the factor orientation and must match the original Cholesky
call. Input `[...,n,n]` produces `[...,n,n]`, preserving batch dimensions.

## Gradient

Reverse mode applies the inverse rule through `L L^T` or `U^T U` and returns a
sensitivity only in the factor's active triangle.

## Example

```cpp
auto a = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
auto l = a.linearalg.cholesky();
auto inverse = l.linearalg.cholesky_inverse();
std::cout << inverse << "\n";
std::cout << a.linearalg.mm(inverse) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.375, -0.25],
  [-0.25, 0.5]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [1.6653345e-16, 1]]
```

The tiny off-diagonal residual is floating-point roundoff, not a failure of
the inverse identity.

## Gotchas And Failure Modes

- Pass the factor, not the original SPD matrix.
- `upper` changes the represented product; it is not merely an output-format toggle.
- An inverse is more expensive and often less stable than solving directly.

## See Also

- [`cholesky`](cholesky.md): compute the factor.
- [`cholesky_solve`](cholesky_solve.md): solve without forming an inverse.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cholesky.html"><span class="label">Previous</span><span class="title">cholesky</span></a>
  <a class="next-link" href="cholesky_solve.html"><span class="label">Next</span><span class="title">cholesky_solve</span></a>
</div>
