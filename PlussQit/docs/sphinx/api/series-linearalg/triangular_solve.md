<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::triangular_solve

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::triangular_solve(
    const Series& b, bool upper = true, bool unitriangular = false) const
Series SeriesLinearAlgProxy::triangular_solve(
    bool upper = true, bool unitriangular = false) const
```

## What It Does

Solves `A X = B` when the owning `Series` is triangular. The method uses
backward substitution for an upper triangle and forward substitution for a
lower triangle, avoiding a general matrix factorization.

## Solving One Known Variable At A Time

Triangular structure orders the unknowns. In a lower-triangular system the
first equation determines the first variable, then each following equation
uses values already found. Upper-triangular systems proceed in reverse. This
is why triangular solves are the final stage of QR, Cholesky, and LU methods.

## Parameters

**b** (`const Series&`)
    Vector `[...,n]` or matrix `[...,n,k]` of right-hand sides. A vector result
    is represented as a column with shape `[...,n,1]`.

**upper** (`bool`, default `true`)
    Read the upper triangle when true; read the lower triangle when false.

**unitriangular** (`bool`, default `false`)
    Treat every diagonal entry as exactly one. Stored diagonal values are
    ignored, so no division by them occurs.

Batch dimensions of `A` and `B` broadcast; matrix dimensions do not.

## Gradient

For incoming sensitivity `G`, `dB = A^-T G` and `dA = -(A^-T G) X^T`, restricted
to the triangle actually used. Both the factor and right-hand side can track
slopes; ignored unit diagonals receive no sensitivity.

## Examples

Example 1. Solve a lower-triangular system with a vector right-hand side.

```cpp
auto l = pluss::make_series({{2.0, 0.0}, {1.0, 3.0}});
auto b = pluss::make_series({2.0, 7.0});
std::cout << l.linearalg.triangular_solve(b, false) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
```

Example 2. Unit-triangular mode ignores deliberately nonsensical diagonal storage.

```cpp
auto l = pluss::make_series({{99.0, 0.0}, {2.0, -7.0}});
std::cout << l.linearalg.triangular_solve(
    pluss::make_series({3.0, 8.0}), false, true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [2]]
```

## Gotchas And Failure Modes

- The default is `upper=true`; pass `false` for a Cholesky lower factor.
- Entries outside the selected triangle are ignored, not validated as zero.
- A zero used diagonal makes the system singular unless unit-triangular mode bypasses it.
- Vector right-hand sides gain a trailing singleton dimension.

## See Also

- [`qr_r`](qr_r.md): produce an upper-triangular factor.
- [`cholesky_solve`](cholesky_solve.md): perform both triangular stages for an SPD system.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="svd_vh.html"><span class="label">Previous</span><span class="title">svd_vh</span></a>
  <a class="next-link" href="vdot.html"><span class="label">Next</span><span class="title">vdot</span></a>
</div>
