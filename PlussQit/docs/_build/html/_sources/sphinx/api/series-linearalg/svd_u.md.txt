<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::svd_u

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::svd_u(bool full_matrices = true) const
Series SeriesLinearAlgProxy::svd_u(bool full_matrices = true) const
```

## What It Does

`svd_u()` returns the left singular vectors: orthonormal output-space
directions in `A = U diag(S) Vh`. Column `i` is the direction produced when the
matching right singular direction is transformed and scaled by `S_i`.

Use `svd_u()` when downstream work needs only an orthonormal basis for the
matrix's output or column space. Typical uses include projecting output-space
observations, comparing learned subspaces, or retaining leading output
directions. Use `svd()` when `U` must be paired with singular values or right
singular vectors.

## Calling Styles And Which One To Use

```cpp
auto direct = matrix.linearalg.svd_u(false);    // SeriesLinearAlgAccessor
auto proxied = matrix.linearalg().svd_u(false); // SeriesLinearAlgProxy
```

Both calls decompose `matrix`. The proxy is merely a lightweight unary wrapper
bound to the accessor's owner; it does not represent a different `U` or a
different algorithm. Prefer the direct accessor spelling. Peer binding through
`linearalg(other)` is for binary operations and is unnecessary here.

## Directions In The Output Space

Reduced `U` describes only directions reached by the represented map. Full `U`
extends those columns to a complete orthonormal basis, including output-space
directions that do not participate in reconstructing a tall matrix.

## Parameter And Shape

For `[...,m,n]` and `k=min(m,n)`, `full_matrices=true` returns `[...,m,m]`;
`false` returns `[...,m,k]`.

## Returns

`Series`
    The left singular-vector matrix. Columns are ordered to match descending
    singular values. The method returns the component directly, unlike
    `svd()`, where the same value is available as `.u`.

## Gradient

Use `full_matrices=false` for reverse mode. Gradients through singular vectors
are unstable at repeated or nearly repeated singular values because no unique
basis exists within the tied subspace.

## Examples

Example 1. Reduced and full mode answer different basis questions for a tall
matrix.

```cpp
auto a = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
std::cout << "reduced=" << a.linearalg.svd_u(false) << "\n";
std::cout << "full=" << a.linearalg.svd_u() << "\n";
```

### Output

```text
reduced=pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0],
  [0, 1],
  [0, 0]]
full=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
```

Reduced `U` spans the represented two-dimensional column space. Full `U` adds
the third orthogonal direction needed to span the complete output space.

Example 2. Validate `U` through orthonormality, not exact signs.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
auto u = matrix.linearalg.svd_u(false);
std::cout << u.transpose(0, 1).linearalg.mm(u) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1.9967346e-16],
  [1.9967346e-16, 1]]
```

Example 3. Reverse mode can differentiate a reduced singular-vector objective.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {0.0, 2.0}});
matrix.tracks_slope_(true);
matrix.linearalg.svd_u(false).sum().reverse();
std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.13374653, -0.089164353],
  [-0.2006198, -0.15603762]]
```

This example uses distinct singular values. At a tie, the individual columns
can rotate inside their shared subspace, so this derivative is not unique.

## Gotchas And Failure Modes

- A column and its matching row in `Vh` may both change sign without changing `A`.
- Full mode adds basis vectors that are not needed for reconstruction.
- Do not compare singular vectors element by element when validating a decomposition.
- `svd_u()` returns a `Series`; `svd()` returns the same coordinated component
  in its `.u` field.

## Summary

Use `svd_u(false)` for the compact orthonormal basis of the output/column
space. Request full mode only when a complete output-space basis is required,
and use joint `svd()` when directions must remain paired with strengths.

## See Also

- [`svd`](svd.md): obtain coordinated factors.
- [`svd_vh`](svd_vh.md): input-space singular directions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="svd_s.html"><span class="label">Previous</span><span class="title">svd_s</span></a>
  <a class="next-link" href="svd_vh.html"><span class="label">Next</span><span class="title">svd_vh</span></a>
</div>
