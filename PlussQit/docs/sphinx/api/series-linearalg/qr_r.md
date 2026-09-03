<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::qr_r

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::qr_r(bool reduced = true) const
Series SeriesLinearAlgProxy::qr_r(bool reduced = true) const
```

## What It Does

`qr_r()` returns the upper-trapezoidal factor `R` from `A = Q R`. Once `Q`
chooses orthonormal directions, `R` contains the coordinates of each input
column in that basis.

## Coordinates After Orthogonalization

The diagonal of `R` records how much new independent direction each column
contributes. A zero or tiny diagonal value indicates exact or near linear
dependence. Because `R` is triangular, QR-based algorithms can finish with an
efficient triangular solve.

## Parameter And Shapes

For input `[...,m,n]`, let `k=min(m,n)`. Reduced mode returns `[...,k,n]`;
full mode returns `[...,m,n]`. For tall matrices, full mode appends zero rows.

## Gradient

Reverse mode is supported for `reduced=true`, jointly accounting for how both
QR factors change even though this method returns only `R`.

## Examples

Example 1. Multiplying matching reduced factors reconstructs the input.

```cpp
auto a = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
auto q = a.linearalg.qr_q();
auto r = a.linearalg.qr_r();
std::cout << r << "\n";
std::cout << q.linearalg.mm(r) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.4142136, -0.70710678],
  [0, -1.2247449]]
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, -2.4682287e-16],
  [0, 1],
  [1, 1]]
```

Example 2. Full `R` matches full `Q` by adding a zero row.

```cpp
auto a = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
std::cout << a.linearalg.qr_r(false) << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.4142136, -0.70710678],
  [0, -1.2247449],
  [0, 0]]
```

## Gotchas And Failure Modes

- Pair `qr_r(reduced)` with `qr_q(reduced)` using the same toggle.
- Signs on diagonal entries are not a portable convention.
- Tiny diagonal values reveal an ill-conditioned basis and unstable downstream solves.

## See Also

- [`qr_q`](qr_q.md): orthonormal basis factor.
- [`triangular_solve`](triangular_solve.md): solve with a triangular factor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qr_q.html"><span class="label">Previous</span><span class="title">qr_q</span></a>
  <a class="next-link" href="slogdet.html"><span class="label">Next</span><span class="title">slogdet</span></a>
</div>
