<!-- pluss-hand-authored -->

# Series::addbmm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::addbmm(const Series& batch1, const Series& batch2, double beta=1.0, double alpha=1.0) const
Series& Series::addbmm_(const Series& batch1, const Series& batch2, double beta=1.0, double alpha=1.0) const
```

## What It Does

`addbmm()` accumulates a *batch* of matrix multiplications by **summing over the batch dimension** into a single 2D result:

```
addbmm(self, batch1, batch2, beta, alpha) = beta * self + alpha * sum_i(batch1[i] @ batch2[i])
```

`batch1` is `(b, n, m)`, `batch2` is `(b, m, p)`, and `self`/the result are `(n, p)` — the batch dimension disappears into the sum. Compare with [`baddbmm()`](baddbmm.md), which keeps the batch dimension instead of collapsing it.

`addbmm_()` is the in-place sibling: it writes the result back into `self` directly — mutating it (and any other handle sharing the same storage) instead of allocating a new `Series` — and returns a reference to `self`. Despite being declared `const` at the call site (a quirk of how the wrapper is generated), it genuinely mutates `self`.

## Parameters

**batch1**, **batch2** (`const Series&`) — the batched operands, `(b, n, m)` and `(b, m, p)`.

**beta**, **alpha** (`double`, default `1.0`) — scale `self` and the summed product respectively.

## Returns

`Series` (out-of-place) or `Series&` (in-place) — the `(n, p)` result.

## Examples

Example 1. Two batches of `2x2` identities and data matrices, summed after multiplication.

```cpp
auto x = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
auto batch1 = pluss::make_series({{{1.0, 0.0}, {0.0, 1.0}}, {{1.0, 0.0}, {0.0, 1.0}}});
auto batch2 = pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}}, {{1.0, 1.0}, {1.0, 1.0}}});

x.addbmm_(batch1, batch2, 1.0, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 3],
  [4, 5]]
```

## Gotchas And Failure Modes

- The batch dimension is summed away — the result is always 2D, never 3D.
- Compare with `baddbmm()`/`baddbmm_()` when you need the per-batch results kept separate.

## Summary

Use `addbmm()`/`addbmm_()` when you want the sum of several matrix products folded into one 2D accumulator in a single call, rather than looping and adding manually.

## See Also

- [`Series::baddbmm`](baddbmm.md): the batch-preserving counterpart.
- [`Series::addmm`](addmm.md): the non-batched two-matrix form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="addcdiv.html">
    <span class="label">Next</span>
    <span class="title">addcdiv</span>
  </a>
</div>
