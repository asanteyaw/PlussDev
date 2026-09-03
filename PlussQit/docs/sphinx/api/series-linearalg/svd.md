<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::svd

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
SvdDecomposition SeriesLinearAlgAccessor::svd(bool full_matrices = true) const
SvdDecomposition SeriesLinearAlgProxy::svd(bool full_matrices = true) const
```

## What It Does

`svd()` returns the three coordinated factors in

```text
A = U diag(S) Vh
```

The returned `SvdDecomposition` has three public `Series` fields:

- `.u`: orthonormal directions in the output space
- `.s`: nonnegative strengths, ordered largest to smallest
- `.vh`: orthonormal input directions, stored as rows

Use this joint method when the factors must be used together: reconstruction,
low-rank approximation, basis conversion, or an algorithm that needs both
directions and strengths. Use `svd_u()`, `svd_s()`, or `svd_vh()` when the code
consumes only one component and returning a decomposition object would obscure
that intent.

## Calling Styles And The Two Signature Types

The two signatures do not perform different decompositions. They describe two
ways the accessor machinery reaches the same unary operation:

```cpp
auto direct = matrix.linearalg.svd(false);    // SeriesLinearAlgAccessor
auto proxied = matrix.linearalg().svd(false); // SeriesLinearAlgProxy
```

`matrix.linearalg` is the accessor stored on `matrix`. Calling `linearalg()`
creates a lightweight proxy bound to that same matrix. Because SVD is unary,
the proxy does not need a peer and both expressions decompose `matrix` with the
same `full_matrices` setting. Users normally write the shorter direct form.

The peer-bound form `matrix.linearalg(other)` exists for binary methods such as
`matmul()` and `lu_solve()`. Do not bind an unrelated peer for `svd()`; SVD has
only one numeric input.

## Rotation, Scaling, Rotation

Read the factorization from right to left. `Vh` first expresses an input in
principal input directions. `diag(S)` independently scales those coordinates.
`U` then expresses the scaled result in output coordinates.

A singular value near zero identifies a direction almost erased by the map.
Exactly zero means rank deficiency: different inputs can produce the same
output. The ratio between the largest and smallest retained singular values
describes how strongly reversing the map can amplify error.

## Parameter

**full_matrices** (`bool`, default `true`)
    Controls whether `U` and `Vh` include complete orthonormal bases or only
    the directions needed to reconstruct `A`. It never changes `.s`.

For input `[...,m,n]`, let `k=min(m,n)`:

| mode | `.u` | `.s` | `.vh` |
| --- | --- | --- | --- |
| `true` | `[...,m,m]` | `[...,k]` | `[...,n,n]` |
| `false` | `[...,m,k]` | `[...,k]` | `[...,k,n]` |

Reduced mode is usually the practical choice: it reconstructs the complete
input with fewer stored values and is the mode supported by reverse mode.
Full mode is useful when an algorithm explicitly needs bases for left or right
null-space complements.

## Returns

`SvdDecomposition`
    A value object containing coordinated `.u`, `.s`, and `.vh` fields. It is
    not itself a `Series`; select a field before applying `Series` methods.

## Gradient

Reverse mode is supported with `full_matrices=false`. A loss may depend on one
or several returned fields, and sensitivities propagate to the original
matrix.

Singular-vector derivatives require distinct singular values. When two values
are equal, their shared subspace is defined but no unique pair of basis vectors
is. Near equality therefore makes gradients through `.u` or `.vh` unstable.
Objectives that depend only symmetrically on `.s` are generally better behaved.

## Examples

Example 1. Direct-accessor and unary-proxy calls are equivalent.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
auto direct = matrix.linearalg.svd(false);
auto proxied = matrix.linearalg().svd(false);

std::cout << "direct_s=" << direct.s << "\n";
std::cout << "proxy_s=" << proxied.s << "\n";
```

### Output

```text
direct_s=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 2]
proxy_s=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 2]
```

The class names in the signatures explain routing; they are not two algorithms
the caller must choose between.

Example 2. Reconstruct a coupled matrix from reduced factors.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
auto parts = matrix.linearalg.svd(false);
auto scale = parts.s.diag_embed();
auto reconstructed = parts.u.linearalg.mm(scale).linearalg.mm(parts.vh);

std::cout << reconstructed << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 1],
  [1, 3]]
```

`diag_embed()` turns the singular-value vector into the middle `k x k` scale
matrix. This multiplication is the operational meaning of the factorization.

Example 3. The singular directions are orthonormal.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
auto parts = matrix.linearalg.svd(false);

std::cout << "u=" << parts.u.transpose(0, 1).linearalg.mm(parts.u) << "\n";
std::cout << "vh=" << parts.vh.linearalg.mm(parts.vh.transpose(0, 1)) << "\n";
```

### Output

```text
u=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1.9967346e-16],
  [1.9967346e-16, 1]]
vh=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 4.2664216e-17],
  [4.2664216e-17, 1]]
```

The tiny off-diagonal values are floating-point roundoff. Orthogonality and
reconstruction are reliable validation criteria; exact vector signs are not.

Example 4. On a tall `3 x 2` matrix, reduced and full mode differ in `U`.

```cpp
auto tall = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
auto reduced = tall.linearalg.svd(false);
auto full = tall.linearalg.svd(true);

std::cout << "reduced_u=" << reduced.u << "\n";
std::cout << "full_u=" << full.u << "\n";
std::cout << "s=" << reduced.s << "\n";
```

### Output

```text
reduced_u=pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0],
  [0, 1],
  [0, 0]]
full_u=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
s=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3, 2]
```

The extra full-mode column spans an output direction not reached by this map.

## Gotchas And Failure Modes

- Input must have at least two dimensions; the last two are matrix dimensions.
- Leading dimensions are batches and are decomposed independently.
- Singular-vector signs are arbitrary. Paired sign changes in `U` and `Vh`
  leave reconstruction unchanged.
- Repeated singular values make individual singular vectors non-unique.
- Full factors consume more memory and are not supported by reverse mode.
- `SvdDecomposition` is a three-field result, not a `Series`.

## Summary

Use `svd(false)` when an algorithm needs coordinated reduced factors. Read
`.vh` as input directions, `.s` as their strengths, and `.u` as the resulting
output directions. Use full mode only when complete null-space bases are part
of the task.

## See Also

- [`svd_u`](svd_u.md): return only output-space directions.
- [`svd_s`](svd_s.md): return only ordered singular strengths.
- [`svd_vh`](svd_vh.md): return only input-space directions.
- [`pinverse`](pinverse.md): reverse directions whose singular values survive a cutoff.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slogdet.html"><span class="label">Previous</span><span class="title">slogdet</span></a>
  <a class="next-link" href="svd_s.html"><span class="label">Next</span><span class="title">svd_s</span></a>
</div>
