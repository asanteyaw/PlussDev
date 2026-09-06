<!-- pluss-hand-authored -->

# Series::bmm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bmm(const Series& other) const -> Series
```

## What It Does

`bmm()` is a **batched** matrix product. Both operands are rank-3 —
`[batch, m, k]` and `[batch, k, n]` — and it multiplies the two matrices in
each batch slot independently, returning `[batch, m, n]`.

Unlike [`matmul()`](../series-linearalg/matmul.md), `bmm()` does **not**
broadcast the batch dimension or accept other ranks: both inputs must be
exactly rank 3 with the same leading size. That strictness is the point — it
rejects an accidental rank mismatch instead of silently reinterpreting it.

For a bias-accumulating batched product use [`baddbmm()`](baddbmm.md).

## How Batch Axes Are Paired

The leading index selects a pair of matrices; it is not part of either matrix
product. For `left[b, i, r]` and `right[b, r, j]`:

```text
result[b, i, j] = sum_r left[b, i, r] * right[b, r, j]
```

Batch slot zero is multiplied only with slot zero, slot one only with slot
one, and so forth. A size-one batch is not repeated. Use `matmul()` when batch
broadcasting is intentional.

## Parameters

**other** (`const Series&`)

The right operand, `[batch, k, n]`. Its batch size must match the receiver's,
and its row count `k` must match the receiver's column count.

## Returns

`Series`
    The `[batch, m, n]` stack of per-batch products.

## Gradient

`bmm()` is differentiable in both operands, per batch:

```
d/d(self)  = g @ other^T      (within each batch)
d/d(other) = self^T @ g       (within each batch)
```

## Examples

Example 1. Two `2 x 2` matrices multiplied per batch.

```cpp
std::vector<double> d(8);
for (int i = 0; i < 8; ++i) d[i] = i + 1;
auto x = pluss::make_series(d, {2, 2, 2});   // batch 0: [[1,2],[3,4]], batch 1: [[5,6],[7,8]]

std::cout << x.bmm(x);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[7, 10],
  [15, 22]],
  [[67, 78],
  [91, 106]]]
```

Batch 0 is `[[1,2],[3,4]] @ [[1,2],[3,4]]`; batch 1 is `[[5,6],[7,8]]` squared.

Example 2. The gradient flows through each batch independently.

```cpp
std::vector<double> d(8);
for (int i = 0; i < 8; ++i) d[i] = i + 1;
auto x = pluss::make_series(d, {2, 2, 2});
x.tracks_slope_(true);

x.bmm(x).sum().reverse();

std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[7, 11],
  [9, 13]],
  [[23, 27],
  [25, 29]]]
```

Because `x` appears on both sides, these values add its left-factor and
right-factor contributions. The two batches still remain independent.

## Gotchas And Failure Modes

- Both operands must be **exactly rank 3**. For rank-polymorphic or
  batch-broadcasting behavior use
  [`matmul()`](../series-linearalg/matmul.md).
- Batch sizes must match; inner dimensions must be conformable.
- Differentiable in both operands.

## Summary

`bmm(other)` multiplies `[b, m, k]` by `[b, k, n]` batch-wise into `[b, m, n]`:

- strict rank-3 contract, no batch broadcasting
- differentiable in both operands
- add a bias with [`baddbmm()`](baddbmm.md)

## See Also

- [`Series::baddbmm`](baddbmm.md): batched `bmm` with a scaled bias.
- [`SeriesLinearAlgAccessor::matmul`](../series-linearalg/matmul.md): the
  rank-polymorphic product with batch broadcasting.
- [`SeriesLinearAlgAccessor::mm`](../series-linearalg/mm.md): the strict 2-D
  product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="block.html">
    <span class="label">Previous</span>
    <span class="title">block</span>
  </a>
  <a class="next-link" href="broadcast_to.html">
    <span class="label">Next</span>
    <span class="title">broadcast_to</span>
  </a>
</div>
