<!-- pluss-hand-authored -->

# Series::addmv

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::addmv(const Series& mat, const Series& vec,
                     double beta = 1.0, double alpha = 1.0) const
Series& Series::addmv_(const Series& mat, const Series& vec, double beta=1.0, double alpha=1.0) const
```

## What It Does

`addmv()` is [`addmm()`](addmm.md) for a matrix-times-vector product:

```
addmv(input, mat, vec) = beta * input + alpha * (mat @ vec)
```

`mat` is `[m, k]`, `vec` is `[k]`, the product is `[m]`, and the receiver
`input` is the length-`m` bias. See [`addmm()`](addmm.md) for the full
description of `beta` / `alpha`, the gradient rules (identical, one rank lower),
and the failure modes.

The receiver may be scalar, in which case one value is added to all `m`
outputs. The matrix and vector do not broadcast: their ranks are fixed and the
matrix's column count must equal the vector's length.

## Parameters

**mat** (`const Series&`) — the `[m, k]` matrix.

**vec** (`const Series&`) — the `[k]` vector; its length must equal `mat`'s
column count.

**beta**, **alpha** (`double`) — scales on `input` and on `mat @ vec`.

## Returns

`Series`
    The length-`m` result `beta * input + alpha * (mat @ vec)`.

## Gradient

Differentiable in `input`, `mat`, and `vec`: `beta * g` to `input`,
`alpha * outer(g, vec)` to `mat`, `alpha * (mat^T @ g)` to `vec`.

## In-Place Form

`addmv_()` is the in-place sibling of [`addmv()`](addmv.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `addmv_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 0.0}, {2});
auto mat = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto vec = pluss::make_series({1.0, 1.0}, {2});

x.addmv_(mat, vec, 1.0, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3, 7]
```

## Examples

```cpp
auto bias = pluss::make_series({10.0, 20.0});
auto mat = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto vec = pluss::make_series({1.0, 1.0, 1.0});

std::cout << bias.addmv(mat, vec);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[16, 35]
```

`mat @ vec` is `[6, 15]`; adding `bias` gives `[16, 35]`.

Example 2. A scalar receiver is shared by every row.

```cpp
auto base = pluss::make_series(4.0);
auto mat = pluss::make_series({{2.0, 0.0}, {1.0, 3.0}});
auto vec = pluss::make_series({5.0, 2.0});
std::cout << base.addmv(mat, vec, 0.5, 2.0);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[22, 24]
```

The product is `[10,11]`; doubling it and adding the broadcast base
contribution `2` produces `[22,24]`.

## Gotchas And Failure Modes

- `mat` must be rank 2 and `vec` rank 1.
- Only the receiver broadcasts to the length-`m` result.
- Receiver, matrix, and vector must share dtype and platform.
- A scalar receiver's sensitivity sums all output sensitivities, then applies
  `beta`.

## Summary

`addmv(mat, vec, beta, alpha)` is the matrix-vector case of
[`addmm()`](addmm.md): `beta * input + alpha * (mat @ vec)`, fully
differentiable.

## See Also

- [`Series::addmm`](addmm.md): the matrix-matrix form and the shared contract.
- [`Series::addr`](addr.md): the outer-product form.
- [`Series::LinearAlgAccessor::mv`](../series-linearalg/mv.md): the bare
  matrix-vector product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addmm.html">
    <span class="label">Previous</span>
    <span class="title">addmm</span>
  </a>
  <a class="next-link" href="addr.html">
    <span class="label">Next</span>
    <span class="title">addr</span>
  </a>
</div>
