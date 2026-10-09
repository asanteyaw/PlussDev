<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::qr_q

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::qr_q(bool reduced = true) const
Series SeriesLinearAlgProxy::qr_q(bool reduced = true) const
```

## What It Does

`qr_q()` returns the orthonormal factor `Q` from `A = Q R`. Its columns form an
orthonormal basis for the column space represented by `A`.

## Building An Orthonormal Basis

QR separates direction from coordinates. `Q` supplies perpendicular unit
directions; `R` records how the original columns combine those directions.
Test a decomposition through `Q R = A` and `Q^T Q = I`, not by expecting fixed
signs: multiplying one column of `Q` and the matching row of `R` by `-1` leaves
the same valid factorization.

## Parameter And Shapes

Let `k=min(m,n)` for input `[...,m,n]`.

| `reduced` | `Q` shape |
| --- | --- |
| `true` | `[...,m,k]` |
| `false` | `[...,m,m]` |

Reduced form stores only the basis needed to reconstruct `A`. Full form extends
it to a complete basis of the ambient row space.

## Gradient

Reverse mode is supported for `reduced=true`. Full `Q` contains an arbitrary
orthonormal completion outside the represented column space and is therefore
not exposed as a differentiable choice.

## Examples

Example 1. Reduced `Q` has orthonormal columns.

```cpp
auto a = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
auto q = a.linearalg.qr_q();
std::cout << q << "\n";
std::cout << q.transpose(0, 1).linearalg.mm(q) << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-0.70710678, 0.40824829],
  [-0, -0.81649658],
  [-0.70710678, -0.40824829]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 8.6508035e-17],
  [8.6508035e-17, 1]]
```

Example 2. Full mode adds one orthonormal completion column for this tall matrix.

```cpp
auto a = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
std::cout << a.linearalg.qr_q(false) << "\n";
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[-0.70710678, 0.40824829, -0.57735027],
  [-0, -0.81649658, -0.57735027],
  [-0.70710678, -0.40824829, 0.57735027]]
```

## Gotchas And Failure Modes

- QR signs can differ while reconstruction and orthogonality remain correct.
- Rank-deficient inputs do not define a stable unique basis.
- Use reduced mode when gradients are required.

## See Also

- [`qr_r`](qr_r.md): triangular coordinate factor.
- [`svd_u`](svd_u.md): left singular directions ordered by singular value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pinverse.html"><span class="label">Previous</span><span class="title">pinverse</span></a>
  <a class="next-link" href="qr_r.html"><span class="label">Next</span><span class="title">qr_r</span></a>
</div>
