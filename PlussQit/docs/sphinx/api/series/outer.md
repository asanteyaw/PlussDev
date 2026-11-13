<!-- pluss-hand-authored -->

# Series::outer

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::outer(const Series& other) const
```

[`ger()`](ger.md) is an exact alias.

## What It Does

`outer()` is the **outer product** of two vectors. The receiver is `[m]`,
`other` is `[n]`, and the result is the `[m, n]` matrix

```
outer(a, b)[i, j] = a[i] * b[j]
```

It is `a.unsqueeze(1).matmul(b.unsqueeze(0))` — a rank-1 matrix built from two
vectors. To add it to an existing matrix in one call, use
[`addr()`](addr.md).

No axis is contracted. Each pair of positions survives as its own output
entry, so `m+n` input positions produce `m*n` output positions. This makes
`outer()` useful for pairwise weights and separable grids, but wrong when the
intended answer is one similarity score.

## Parameters

**other** (`const Series&`)

The second vector, `[n]`. Both operands must be one-dimensional.

## Returns

`Series`
    The `[m, n]` outer-product matrix.

## Gradient

`outer()` is differentiable in both operands: for an upstream `g` (shape
`[m, n]`),

```
d/d(self)  = g @ other
d/d(other) = g^T @ self
```

## Examples

Example 1. The `[m, n]` grid of pairwise products.

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({3.0, 4.0, 5.0});

std::cout << a.outer(b);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 4, 5],
  [6, 8, 10]]
```

Row `i` is `a[i] * b`.

Example 2. The gradient with respect to the first vector.

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({3.0, 4.0, 5.0});
a.tracks_slope_(true);

a.outer(b).sum().reverse();

std::cout << "dL/da=" << a.sensitivity();
```

### Output

```text
dL/da=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[12, 12]
```

With `g = ones(2, 3)`, `dL/da = g @ b = [3+4+5, 3+4+5] = [12, 12]`.

## Gotchas And Failure Modes

- Both operands must be 1-D.
- Their lengths may differ; there is no requirement that `m == n`.
- `outer()` and [`ger()`](ger.md) are the same call.
- For "matrix plus outer product" use [`addr()`](addr.md) rather than
  `M.add(a.outer(b))`.

## Summary

`outer(other)` is the rank-1 matrix `a[i] * b[j]`:

- `[m]` and `[n]` in, `[m, n]` out
- differentiable in both vectors
- `ger` is an alias; `addr` is the accumulating form

## See Also

- [`Series::ger`](ger.md): the BLAS-named alias.
- [`Series::addr`](addr.md): add a scaled outer product to an existing matrix.
- [`Series::inner`](inner.md): the complementary contraction to a scalar.
- [`Series::LinearAlgAccessor::dot`](../series-linearalg/dot.md): the vector inner
  product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numel.html">
    <span class="label">Previous</span>
    <span class="title">numel</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
