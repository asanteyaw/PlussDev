<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::svd_s

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::svd_s(bool full_matrices = true) const
Series SeriesLinearAlgProxy::svd_s(bool full_matrices = true) const
```

## What It Does

`svd_s()` returns the singular values of every matrix, ordered largest first.
They are the nonnegative axis scales in `A = U diag(S) Vh` and reveal effective
rank without requiring singular vectors.

Use `svd_s()` when the question is about strength rather than direction:
spectral size, conditioning, numerical rank, rank penalties, or deciding which
directions a pseudoinverse should retain. Use `svd()` instead when the singular
values must remain coordinated with `U` or `Vh` for reconstruction or basis
conversion.

## Calling Styles And Which One To Use

The two signature types expose the same unary operation:

```cpp
auto direct = matrix.linearalg.svd_s(false);    // accessor spelling
auto proxied = matrix.linearalg().svd_s(false); // unary proxy spelling
```

Both calls read `matrix` and return the same singular values. The accessor is
the `linearalg` member attached to the matrix. Adding `()` creates a lightweight
`SeriesLinearAlgProxy` bound to that same matrix; it does not introduce a
second operand. Prefer the shorter direct spelling in ordinary code. The proxy
spelling mainly exists so the accessor family has one uniform routing model.

Do not confuse this with `matrix.linearalg(other)`, which binds a peer for
binary methods. `svd_s()` is unary and does not need a peer.

## Measuring Directional Strength

The largest singular value is the strongest amplification performed by `A`.
Small values identify directions that are nearly erased. Their ratio controls
conditioning, while counting values above a tolerance gives a numerical rank.

## Parameter And Shape

For `[...,m,n]`, the result is always `[...,min(m,n)]`. `full_matrices` is
accepted for a uniform SVD interface but does not change singular values or
their shape.

Consequently, `svd_s()` normally should be called as `svd_s(false)`: it states
the reduced-SVD intent needed by reverse mode. Passing `true` produces the same
value and shape for `S`, but asks for the full decomposition mode internally.

## Returns

`Series`
    Singular values with shape `[...,k]`, where `k=min(m,n)`. Unlike `svd()`,
    this method returns a `Series` directly, so it can be reduced, compared,
    indexed, or differentiated without selecting a field first.

## Gradient

For distinct singular values, a sensitivity on `S_i` contributes along the
outer product of the matching left and right singular vectors. Repeated values
make a particular vector basis ambiguous, though symmetric singular-value
objectives can remain meaningful.

## Examples

Example 1. Values are sorted by strength.

```cpp
auto a = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
std::cout << a.linearalg.svd_s(false) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3, 2]
```

Example 2. Accessor and unary-proxy spellings are equivalent.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
std::cout << "accessor=" << matrix.linearalg.svd_s(false) << "\n";
std::cout << "proxy=" << matrix.linearalg().svd_s(false) << "\n";
```

### Output

```text
accessor=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 2]
proxy=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 2]
```

Example 3. A rank-deficient matrix exposes one numerically zero direction.

```cpp
auto rank_one = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
std::cout << rank_one.linearalg.svd_s(false) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 1.8144368e-16]
```

The second value is roundoff-scale rather than exactly zero. Numerical rank
therefore requires a tolerance relative to the largest value and matrix size.

Example 4. Batches produce one singular-value vector per matrix.

```cpp
auto batch = pluss::make_series({
    {{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}},
    {{4.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}},
});
std::cout << batch.linearalg.svd_s(false) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 2],
  [4, 1]]
```

Example 5. A singular-value objective differentiates back to the matrix.

```cpp
auto matrix = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
matrix.tracks_slope_(true);
matrix.linearalg.svd_s(false).sum().reverse();
std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0],
  [0, 1],
  [0, 0]]
```

Here the objective is the sum of singular values. Each active singular
direction contributes its left-right outer product to the input sensitivity.

## Gotchas And Failure Modes

- `full_matrices` does not affect this component's output.
- Near-zero values should be compared with a scale-aware tolerance, not exact equality.
- Singular values alone do not reveal which input directions they describe.
- `svd_s()` returns a `Series`; `svd()` returns an `SvdDecomposition` whose
  singular-value field is `.s`.

## Summary

Use `svd_s(false)` when only directional strengths, rank, or conditioning are
needed. Use `svd(false)` when those strengths must stay paired with singular
vectors. The accessor and unary-proxy spellings are equivalent; prefer the
shorter accessor spelling.

## See Also

- [`svd`](svd.md): obtain all factors together.
- [`norm`](norm.md): one global magnitude rather than directional magnitudes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="svd.html"><span class="label">Previous</span><span class="title">svd</span></a>
  <a class="next-link" href="svd_u.html"><span class="label">Next</span><span class="title">svd_u</span></a>
</div>
