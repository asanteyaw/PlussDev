<!-- pluss-hand-authored -->

# Series::addr

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::addr(const Series& vec1, const Series& vec2,
             double beta = 1.0, double alpha = 1.0) const -> Series
```

## What It Does

`addr()` adds a scaled **outer product** of two vectors to the receiver:

```
addr(input, vec1, vec2) = beta * input + alpha * (vec1 ⊗ vec2)
```

`vec1` is `[m]`, `vec2` is `[n]`, the outer product `vec1 ⊗ vec2` is the
`[m, n]` matrix whose `(i, j)` entry is `vec1[i] * vec2[j]`, and `input`
broadcasts against `[m, n]`. It is the rank-1-update member of the
[`addmm()`](addmm.md) family; see that page for `beta` / `alpha` and the
gradient contract.

The bare outer product with no accumulator is [`outer()`](outer.md).

This is called a rank-one update because the newly formed matrix has rank at
most one when neither vector is zero. The receiver itself is normally a
matrix, but may be a scalar, row, or column shape that broadcasts to `[m,n]`.

## Parameters

**vec1** (`const Series&`) — the `[m]` left vector.

**vec2** (`const Series&`) — the `[n]` right vector.

**beta**, **alpha** (`double`) — scales on `input` and on the outer product.

## Returns

`Series`
    The `[m, n]` result `beta * input + alpha * (vec1 ⊗ vec2)`.

## Gradient

Differentiable in `input`, `vec1`, and `vec2`: `beta * g` to `input`,
`alpha * (g @ vec2)` to `vec1`, `alpha * (g^T @ vec1)` to `vec2`.

## Examples

```cpp
auto base = pluss::make_series({{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}});
auto v1 = pluss::make_series({1.0, 2.0});
auto v2 = pluss::make_series({3.0, 4.0, 5.0});

std::cout << base.addr(v1, v2);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 4, 5],
  [6, 8, 10]]
```

Row `i` is `v1[i] * v2`.

Example 2. A row receiver broadcasts down the outer-product rows.

```cpp
auto row = pluss::make_series({10.0, 20.0, 30.0});
auto u = pluss::make_series({1.0, 2.0});
auto v = pluss::make_series({3.0, 4.0, 5.0});
std::cout << row.addr(u, v, 1.0, 2.0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[16, 28, 40],
  [22, 36, 50]]
```

## Gotchas And Failure Modes

- Both product operands must be one-dimensional; neither one broadcasts.
- The receiver alone may broadcast to the resulting `[m,n]` matrix.
- Receiver and vectors must share dtype and platform.
- This is an outer product, not elementwise vector multiplication.

## Summary

`addr(vec1, vec2, beta, alpha)` is a scaled rank-1 update:
`beta * input + alpha * (vec1 ⊗ vec2)`, fully differentiable.

## See Also

- [`Series::addmm`](addmm.md): the matrix-matrix form and shared contract.
- [`Series::outer`](outer.md): the bare outer product with no accumulator.
- [`Series::addmv`](addmv.md): the matrix-vector form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addmv.html">
    <span class="label">Previous</span>
    <span class="title">addmv</span>
  </a>
  <a class="next-link" href="adjoint.html">
    <span class="label">Next</span>
    <span class="title">adjoint</span>
  </a>
</div>
