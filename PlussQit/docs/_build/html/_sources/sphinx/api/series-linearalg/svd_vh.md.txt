<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::svd_vh

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::svd_vh(bool full_matrices = true) const
Series SeriesLinearAlgProxy::svd_vh(bool full_matrices = true) const
```

## What It Does

`svd_vh()` returns conjugate-transposed right singular vectors from
`A = U diag(S) Vh`. Each row identifies an orthonormal input-space direction;
`A` maps it to the matching column of `U`, scaled by the corresponding singular
value.

Use `svd_vh()` when the task needs only principal directions in the input
space: feature directions, right-null-space analysis, or projection of input
coordinates. Use joint `svd()` when those rows must remain paired with their
singular values or output directions.

## Calling Styles And Which One To Use

```cpp
auto direct = matrix.linearalg.svd_vh(false);    // accessor spelling
auto proxied = matrix.linearalg().svd_vh(false); // unary proxy spelling
```

Both expressions read the same `matrix`. `SeriesLinearAlgProxy` is only the
temporary object returned by `linearalg()`; it does not mean “proxy data” and
does not change the decomposition. Prefer the shorter direct spelling. SVD is
unary, so no peer should be bound through `linearalg(other)`.

## Directions Presented As Rows

The `h` matters: directions are rows of `Vh`, not columns of `V`. For real
inputs this is an ordinary transpose; for complex inputs it also conjugates.
This orientation allows `diag(S) Vh` to appear directly in reconstruction.

## Parameter And Shape

For `[...,m,n]` and `k=min(m,n)`, full mode returns `[...,n,n]`; reduced mode
returns `[...,k,n]`.

## Returns

`Series`
    Right singular directions stored row-wise. The same coordinated component
    is available as `.vh` on the result of `svd()`.

## Gradient

Use reduced mode for reverse differentiation. Repeated singular values make
individual rows non-unique, and near ties can produce large vector gradients.

## Examples

Example 1. A wide matrix shows why reduced and full `Vh` differ.

```cpp
auto wide = pluss::make_series({{3.0, 0.0, 0.0}, {0.0, 2.0, 0.0}});
std::cout << "reduced=" << wide.linearalg.svd_vh(false) << "\n";
std::cout << "full=" << wide.linearalg.svd_vh(true) << "\n";
```

### Output

```text
reduced=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
full=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
```

The extra full-mode row spans the right null space: the input direction that
this `2 x 3` map sends to zero.

Example 2. Rows of reduced `Vh` are orthonormal.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
auto vh = matrix.linearalg.svd_vh(false);
std::cout << vh.linearalg.mm(vh.transpose(0, 1)) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 4.2664216e-17],
  [4.2664216e-17, 1]]
```

Example 3. Reduced-mode singular directions participate in reverse mode.

```cpp
auto matrix = pluss::make_series({{3.0, 1.0}, {0.0, 2.0}});
matrix.tracks_slope_(true);
matrix.linearalg.svd_vh(false).sum().reverse();
std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.25407734, -0.32667087],
  [-0.14518705, -0.21778058]]
```

## Gotchas And Failure Modes

- Rows, not columns, are the right singular directions.
- Signs and bases inside repeated-value subspaces are not unique.
- Pair this output with `S` and `U` produced using the same mode.
- Full mode is not supported by reverse mode.

## Summary

Use `svd_vh(false)` for compact row-wise principal directions in the input
space. Use full mode only when the complete right null-space basis is needed,
and joint `svd()` when directions must stay paired with scales and outputs.

## See Also

- [`svd`](svd.md): obtain coordinated factors.
- [`svd_u`](svd_u.md): output-space directions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="svd_u.html"><span class="label">Previous</span><span class="title">svd_u</span></a>
  <a class="next-link" href="triangular_solve.html"><span class="label">Next</span><span class="title">triangular_solve</span></a>
</div>
