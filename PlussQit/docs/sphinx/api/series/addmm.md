<!-- pluss-hand-authored -->

# Series::addmm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::addmm(const Series& mat1, const Series& mat2,
                     double beta = 1.0, double alpha = 1.0) const
Series& Series::addmm_(const Series& mat1, const Series& mat2, double beta=1.0, double alpha=1.0) const
```

## What It Does

`addmm()` is the fused "bias plus matrix product" that a linear layer is built
from:

```
addmm(input, mat1, mat2) = beta * input + alpha * (mat1 @ mat2)
```

`mat1` is `[m, k]`, `mat2` is `[k, n]`, and the receiver `input` broadcasts
against the `[m, n]` product — usually it is the `[n]` bias row, or the full
`[m, n]` accumulator. `beta` scales the existing `input`, `alpha` scales the new
product; both default to `1`.

It is exactly `input.mul(beta).add(mat1.matmul(mat2).mul(alpha))`, done as one
call. The siblings pin the second factor's shape:

| method | product | second factor |
|---|---|---|
| `addmm` | matrix × matrix | `[k, n]` |
| [`addmv`](addmv.md) | matrix × vector | `[k]` |
| [`addr`](addr.md) | outer product of two vectors | — |
| [`baddbmm`](baddbmm.md) | batched matrix × matrix | `[b, k, n]` |

## Reading The Three Roles

The receiver is not one of the multiplied matrices. In
`input.addmm(mat1, mat2)`, `mat1` supplies rows, `mat2` supplies columns, and
`input` supplies an existing value to add to every output position. This is
why a length-`n` receiver is useful: it is interpreted as one row and repeated
over the `m` result rows by broadcasting.

Only the receiver broadcasts. `mat1` and `mat2` must both be matrices, and
their shared `k` dimension must agree. Use `matmul()` when the factors
themselves need batch broadcasting.

All three operands must also use the same dtype and platform. `addmm()` does
not apply the general arithmetic promotion rules to a differently typed bias;
convert it explicitly before the fused call.

## Parameters

**mat1** (`const Series&`) — the left factor, `[m, k]`.

**mat2** (`const Series&`) — the right factor, `[k, n]`. Its rows must match
`mat1`'s columns.

**beta** (`double`, default `1.0`) — scale on the existing `input`. `beta = 0`
drops `input` entirely (and skips reading it).

**alpha** (`double`, default `1.0`) — scale on the `mat1 @ mat2` product.

## Returns

`Series`
    The `[m, n]` result `beta * input + alpha * (mat1 @ mat2)`.

## Gradient

`addmm()` is differentiable in `input`, `mat1`, and `mat2`. The backward is the
standard matmul rule scaled by `alpha`, plus `beta` straight through to
`input`:

```
d/d(input) = beta * g
d/d(mat1)  = alpha * (g @ mat2^T)
d/d(mat2)  = alpha * (mat1^T @ g)
```

When `input` was broadcast, its sensitivity is reduced back to the receiver's
original shape. For a row bias `[n]` used against an `[m,n]` result, that means
summing the `m` output-row sensitivities. Setting `beta=0` removes the receiver
from both the forward value and its sensitivity and suppresses any `NaN` or
infinity stored there.

## In-Place Form

`addmm_()` is the in-place sibling of [`addmm()`](addmm.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `addmm_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
auto mat1 = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
auto mat2 = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

x.addmm_(mat1, mat2, 1.0, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

## Examples

Example 1. The default `beta = alpha = 1`: bias plus product.

```cpp
auto bias = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});

std::cout << bias.addmm(a, b);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[20, 22],
  [43, 51]]
```

`a @ b` is `[[19, 22], [43, 50]]`; adding the identity `bias` gives the result.

Example 2. `beta` and `alpha` scale the two terms independently.

```cpp
auto bias = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});

std::cout << bias.addmm(a, b, /*beta=*/0.5, /*alpha=*/2.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[38.5, 44],
  [86, 100.5]]
```

`0.5 * bias + 2 * (a @ b)`.

Example 3. The gradient with respect to `mat1`.

```cpp
auto bias = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
a.tracks_slope_(true);

bias.addmm(a, b).sum().reverse();

std::cout << "dL/da=" << a.sensitivity();
```

### Output

```text
dL/da=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[11, 15],
  [11, 15]]
```

With `g = ones(2, 2)`, `dL/da = g @ b^T = [[1,1],[1,1]] @ [[5,7],[6,8]] =
[[11,15],[11,15]]`.

Example 4. A row bias broadcasts over output rows.

```cpp
auto bias = pluss::make_series({10.0, 20.0});
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
std::cout << bias.addmm(a, b, 0.5, 2.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[43, 54],
  [91, 110]]
```

The product is `[[19,22],[43,50]]`; twice that product receives half of the
bias row, `[5,10]`, on each row.

Example 5. Inner dimensions that do not match are rejected.

```cpp
auto bias = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

try {
  bias.addmm(a, pluss::make_series({{1.0, 2.0, 3.0}}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
addmm: inner dimensions must match
```

## Gotchas And Failure Modes

- `mat1`'s column count must equal `mat2`'s row count.
- `beta` and `alpha` are plain scalars, not `Series`.
- The receiver `input` broadcasts against `[m, n]`; a bias vector of length `n`
  is the common case.
- Fully differentiable — this is the intended building block for a trainable
  linear map.

## Summary

`addmm(mat1, mat2, beta, alpha)` fuses a scaled bias and a scaled matrix
product:

- `beta * input + alpha * (mat1 @ mat2)`
- differentiable in all three tensors
- vector / outer / batched variants: [`addmv`](addmv.md), [`addr`](addr.md),
  [`baddbmm`](baddbmm.md)

## See Also

- [`Series::LinearAlgAccessor::mm`](../series-linearalg/mm.md): the bare matrix
  product `addmm` adds a bias to.
- [`Series::addmv`](addmv.md) / [`Series::addr`](addr.md) /
  [`Series::baddbmm`](baddbmm.md): the vector, outer-product, and batched forms.
- [`Series::addcmul`](addcmul.md): the elementwise fused multiply-accumulate.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addcmul.html">
    <span class="label">Previous</span>
    <span class="title">addcmul</span>
  </a>
  <a class="next-link" href="addmv.html">
    <span class="label">Next</span>
    <span class="title">addmv</span>
  </a>
</div>
