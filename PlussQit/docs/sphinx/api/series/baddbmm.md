<!-- pluss-hand-authored -->

# Series::baddbmm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::baddbmm(const Series& batch1, const Series& batch2,
                double beta = 1.0, double alpha = 1.0) const -> Series
Series& Series::baddbmm_(const Series& batch1, const Series& batch2, double beta=1.0, double alpha=1.0) const
```

## What It Does

`baddbmm()` is [`addmm()`](addmm.md) for a **batched** matrix product — the
batched counterpart of [`bmm()`](bmm.md) with a scaled accumulator:

```
baddbmm(input, batch1, batch2) = beta * input + alpha * (batch1 @@ batch2)
```

`batch1` is `[b, m, k]`, `batch2` is `[b, k, n]`, `@@` is the per-batch product
([`bmm`](bmm.md)), and `input` broadcasts against `[b, m, n]`. See
[`addmm()`](addmm.md) for `beta` / `alpha` and the gradient contract; the rule
here is the same, applied per batch.

The product operands are strict rank-3 arrays with equal batch counts; they do
not broadcast. The receiver may broadcast to `[b,m,n]`. A receiver shaped
`[m,n]` is therefore one matrix bias reused by every batch, while `[n]` is one
row bias reused by every row of every batch.

## Parameters

**batch1** (`const Series&`) — `[b, m, k]`.

**batch2** (`const Series&`) — `[b, k, n]`.

**beta**, **alpha** (`double`) — scales on `input` and on the batched product.

## Returns

`Series`
    The `[b, m, n]` result `beta * input + alpha * (batch1 @@ batch2)`.

## Gradient

Differentiable in `input`, `batch1`, and `batch2`, applying the
[`bmm()`](bmm.md) backward per batch and scaling by `alpha`, with `beta`
straight through to `input`.

## In-Place Form

`baddbmm_()` is the in-place sibling of [`baddbmm()`](baddbmm.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `baddbmm_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{{0.0, 0.0}, {0.0, 0.0}}});
auto batch1 = pluss::make_series({{{1.0, 0.0}, {0.0, 1.0}}});
auto batch2 = pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}}});

x.baddbmm_(batch1, batch2, 1.0, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[1,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[1, 2],
  [3, 4]]]
```

## Examples

```cpp
std::vector<double> d(8);
for (int i = 0; i < 8; ++i) d[i] = i + 1;
auto x = pluss::make_series(d, {2, 2, 2});
auto zero_bias = pluss::make_series(std::vector<double>(8, 0.0), {2, 2, 2});

std::cout << zero_bias.baddbmm(x, x);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[7, 10],
  [15, 22]],
  [[67, 78],
  [91, 106]]]
```

With a zero bias and `beta = alpha = 1`, this is exactly [`bmm()`](bmm.md).

Example 2. One matrix bias is shared by the batch and independently scaled.

```cpp
auto bias = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
auto left = pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}}});
auto identity = pluss::make_series({{{1.0, 0.0}, {0.0, 1.0}}});
std::cout << bias.baddbmm(left, identity, 0.5, 2.0);
```

### Output

```text
pluss::Series(shape=[1,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[[7, 14],
  [21, 28]]]
```

## Gotchas And Failure Modes

- Both factors must be rank 3, with matching batch and inner dimensions.
- Broadcasting applies to the receiver only, after the product exists.
- Receiver and factors must share dtype and platform.
- `beta=0` excludes receiver values, including stored `NaN` or infinity.
- A broadcast receiver receives summed sensitivity over repeated axes.

## Summary

`baddbmm(batch1, batch2, beta, alpha)` is the batched
[`addmm()`](addmm.md): `beta * input + alpha * (batch1 @@ batch2)`, fully
differentiable.

## See Also

- [`Series::bmm`](bmm.md): the bare batched product.
- [`Series::addmm`](addmm.md): the 2-D form and the shared contract.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="atanh.html">
    <span class="label">Previous</span>
    <span class="title">atanh</span>
  </a>
  <a class="next-link" href="beta.html">
    <span class="label">Next</span>
    <span class="title">beta</span>
  </a>
</div>
