<!-- pluss-hand-authored -->

# Series::kron

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::kron(const Series& other) const
```

## What It Does

`kron()` is the **Kronecker product**. For two matrices `A` (`[m, n]`) and `B`
(`[p, q]`) it produces the `[m*p, n*q]` block matrix

```
       | A[0,0]*B   A[0,1]*B   ...  |
A ⊗ B = | A[1,0]*B   A[1,1]*B   ...  |
       |   ...                      |
```

— every entry of `A` scaled by the whole of `B`, tiled into place. It shows up
in tensor-product constructions: building a 2-D operator from two 1-D ones,
expanding a state space, forming separable filters.

`kron()` is computed as a broadcast multiply of reshaped operands
(`A` reshaped to `[m, 1, n, 1]` times `B` reshaped to `[1, p, 1, q]`, folded
back to `[m*p, n*q]`), so it is a plain differentiable expression — no special
backward.

## Parameters

**other** (`const Series&`)

The right operand `B`. For the 2-D form both operands are matrices; other ranks
fall back to the outer product of the flattened operands reshaped to the
concatenation of the two input shapes.

For non-matrix inputs, read that rule literally. If `A` has shape `[a0,a1]`
and `B` has shape `[b0]`, the result shape is `[a0,a1,b0]`; values are ordered
as every flattened value of `A` multiplying every flattened value of `B`.
This differs from the block folding used by the dedicated matrix case.

## Returns

`Series`
    The `[m*p, n*q]` Kronecker product (2-D form).

## Gradient

`kron()` is differentiable in both operands. It is bilinear, so the backward is
the matching contraction of the upstream gradient with the other operand.

## Examples

Example 1. A `2 x 2` Kronecker product.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{0.0, 5.0}, {6.0, 7.0}});

std::cout << a.kron(b);
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[[0, 5, 0, 10],
  [6, 7, 12, 14],
  [0, 15, 0, 20],
  [18, 21, 24, 28]]
```

The top-left `2 x 2` block is `1 * b`, the top-right is `2 * b`, and so on.

Example 2. The gradient with respect to `A`: each `A[i,j]` sees the whole of
`B`.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{0.0, 5.0}, {6.0, 7.0}});
a.tracks_slope_(true);

a.kron(b).sum().reverse();

std::cout << "dL/da=" << a.sensitivity();
```

### Output

```text
dL/da=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[18, 18],
  [18, 18]]
```

With `g = ones`, every `dL/dA[i,j] = sum(B) = 0 + 5 + 6 + 7 = 18`.

Example 3. Vector inputs use the general concatenated-shape rule.

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({10.0, 20.0, 30.0});
std::cout << a.kron(b);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20, 30],
  [20, 40, 60]]
```

## Gotchas And Failure Modes

- The 2-D form is the well-defined case: `[m,n] ⊗ [p,q] -> [m*p, n*q]`.
- Integer inputs are promoted to floating point.
- The result grows multiplicatively — `[100, 100] ⊗ [100, 100]` is a
  `10000 x 10000` matrix.

## Summary

`kron(other)` is the Kronecker (tensor) product:

- `[m,n] ⊗ [p,q] -> [m*p, n*q]`, each `A[i,j]` scaling a full copy of `B`
- a differentiable broadcast-multiply expression
- promotes integer input to floating point

## See Also

- [`Series::outer`](outer.md): the vector special case (`[m] ⊗ [n]`, up to a
  reshape).
- [`Series::LinearAlgAccessor::matmul`](../series-linearalg/matmul.md): the
  ordinary matrix product.
- [`Series::repeat`](repeat.md): plain tiling without the per-block scaling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kthvalue.html">
    <span class="label">Previous</span>
    <span class="title">kthvalue</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
