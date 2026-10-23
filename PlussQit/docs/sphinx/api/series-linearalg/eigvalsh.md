<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::eigvalsh

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::eigvalsh(bool upper = false) const
Series SeriesLinearAlgProxy::eigvalsh(bool upper = false) const
```

## What It Does

`eigvalsh()` returns the eigenvalues of a symmetric (real) or Hermitian
(complex) matrix: the factors by which the matrix stretches its principal
directions. They are real and come in ascending order, so the first entry is the
smallest and the last the largest.

It is the `.values` field of [`eigh`](eigh.md), returned directly as a `Series`.
Use it when the eigenvalues alone answer the question: is the matrix positive
definite, how ill-conditioned is it, what is its spectral norm, what is its
determinant or trace-like invariant, how much variance does the leading
component explain.

The call does not compute fewer things than `eigh()`: it is the same solve, and
the eigenvectors are simply not returned. Call `eigh()` once when both are
needed rather than calling both methods.

Read [`eigh`](eigh.md) for the conventions that carry over unchanged: the matrix
is assumed symmetric or Hermitian and is not checked, only one triangle is read,
the imaginary part of a complex diagonal is ignored, integer input is rejected,
and the solve runs on the CPU.

## Parameter

**upper** (`bool`, default `false`)
    Selects the triangle that is read: `false` for the lower triangle (including
    the diagonal), `true` for the upper one. Both choices agree for an exactly
    symmetric or Hermitian matrix.

## Returns

`Series`
    Shape `[..., n]` for input `[..., n, n]`. The dtype is real:
    `Float64` for `Float64` or `Complex128` input, and `Float32` for `Float32` or
    `Complex64` input. Leading dimensions are batch dimensions.

## Gradient

Reverse mode is supported, for real and complex input and for batches. The
gradient of one eigenvalue with respect to the matrix is the projector `v v^H`
onto its eigenvector. A loss built from all the eigenvalues symmetrically (their
sum, a product, a spectral norm) has a well-defined gradient even when
eigenvalues repeat. A loss that singles out one member of a repeated pair has no
unique gradient, because the eigenvector it would project onto is not unique.

## Examples

Example 1. The eigenvalues of a symmetric matrix, ascending.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}});
pluss::series::print(std::cout, matrix.linearalg.eigvalsh());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 3.0000]
```

The matrix stretches the direction `(1, 1)` by 3 and the direction `(1, -1)` by 1.

Example 2. The same values as the `eigh()` field, and the same values through
the unary proxy.

```cpp
auto matrix = pluss::make_series({{4.0, 1.0, 0.0}, {1.0, 3.0, 1.0}, {0.0, 1.0, 2.0}});
auto direct = matrix.linearalg.eigvalsh();
auto proxied = matrix.linearalg().eigvalsh();
std::cout << std::boolalpha
          << "equals eigh().values: " << direct.equal(matrix.linearalg.eigh().values) << "\n"
          << "proxy gives the same values: " << direct.equal(proxied) << "\n";
pluss::series::print(std::cout, direct);
```

### Output

```text
equals eigh().values: true
proxy gives the same values: true
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.2679, 3.0000, 4.7321]
```

Example 3. Checking positive definiteness. A symmetric matrix is positive
definite exactly when its smallest eigenvalue is positive, and the ascending
order puts that value first.

```cpp
auto positive = pluss::make_series({{2.0, -1.0}, {-1.0, 2.0}});
auto indefinite = pluss::make_series({{1.0, 2.0}, {2.0, 1.0}});
std::cout << std::boolalpha
          << "first matrix is positive definite: " << (positive.linearalg.eigvalsh().select(0, 0).item<double>() > 0) << "\n"
          << "second matrix is positive definite: " << (indefinite.linearalg.eigvalsh().select(0, 0).item<double>() > 0) << "\n";
pluss::series::print(std::cout, indefinite.linearalg.eigvalsh());
```

### Output

```text
first matrix is positive definite: true
second matrix is positive definite: false
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.0000, 3.0000]
```

Example 4. The condition number of a symmetric positive-definite matrix is the
ratio of its extreme eigenvalues.

```cpp
auto matrix = pluss::make_series({{4.0, 0.0}, {0.0, 0.25}});
auto values = matrix.linearalg.eigvalsh();
std::cout << "condition number: "
          << values.select(0, 1).item<double>() / values.select(0, 0).item<double>() << "\n";
```

### Output

```text
condition number: 16
```

Example 5. Choosing the triangle. The matrix below is not symmetric; each
choice of `upper` defines the symmetric matrix that is actually used.

```cpp
auto matrix = pluss::make_series({{2.0, 100.0}, {1.0, 3.0}});
pluss::series::print(std::cout, matrix.linearalg.eigvalsh());
std::cout << "\n";
pluss::series::print(std::cout, matrix.linearalg.eigvalsh(true));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.3820, 3.6180]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-97.5012, 102.5012]
```

Example 6. A complex Hermitian matrix has real eigenvalues, in the real dtype
that matches its precision.

```cpp
using C = std::complex<double>;
auto matrix = pluss::make_series(
    std::vector<C>{{2, 0}, {1, -1}, {1, 1}, {3, 0}}, {2, 2});
auto values = matrix.linearalg.eigvalsh();
pluss::series::print(std::cout, values);
std::cout << "\n" << std::boolalpha
          << "real dtype: " << !values.is_complex() << "\n"
          << "single precision input gives Float32: "
          << (matrix.to(pluss::ScalarType::Complex64).linearalg.eigvalsh().scalar_type() ==
              pluss::ScalarType::Float32) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 4.0000]
real dtype: true
single precision input gives Float32: true
```

Example 7. Batches. A leading dimension gives one row of eigenvalues per matrix.

```cpp
auto stack = pluss::make_series({{{2.0, 0.0}, {0.0, 5.0}},
                                 {{1.0, 2.0}, {2.0, 1.0}},
                                 {{4.0, 0.0}, {0.0, 4.0}}});
pluss::series::print(std::cout, stack.linearalg.eigvalsh());
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  2.0000 5.0000 ⎤
⎢ -1.0000 3.0000 ⎥
⎣  4.0000 4.0000 ⎦
```

Example 8. Reverse mode. The gradient of the sum of the eigenvalues is the
identity (the sum of the eigenvalues is the trace). The gradient of the largest
eigenvalue is the projector onto its eigenvector.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}});
matrix.tracks_slope_(true);
matrix.linearalg.eigvalsh().sum().reverse();
std::cout << std::boolalpha << "gradient of the sum is the identity: "
          << (matrix.sensitivity().sub(pluss::series::eye(2)).abs().max().item<double>() < 1e-14)
          << "\n";

auto again = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}});
again.tracks_slope_(true);
again.linearalg.eigvalsh().select(0, 1).reverse();
pluss::series::print(std::cout, again.sensitivity());
```

### Output

```text
gradient of the sum is the identity: true
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.5000 0.5000 ⎤
⎣ 0.5000 0.5000 ⎦
```

Example 9. Invalid input is rejected.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).linearalg.eigvalsh();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
subsp::math::linalg::eigh: expected shape [..., n, n]
```

## Gotchas And Failure Modes

- **Symmetry is assumed, not checked**, and only one triangle is read.
- **Ascending order**, smallest first. The largest eigenvalue is the last entry.
- **No cheaper than `eigh()`.** Call `eigh()` when the eigenvectors are needed too.
- **Not the spectrum of a general matrix.** A matrix that is not symmetric or
  Hermitian has eigenvalues that this method does not compute.
- **Input must be floating point or complex**; integers are rejected.

## Summary

`eigvalsh()` returns the real, ascending eigenvalues of a symmetric or Hermitian
matrix as a `Series` of shape `[..., n]`, batched over leading dimensions and
differentiable. It is `eigh().values`.

## See Also

- [`eigh`](eigh.md): the eigenvalues together with the eigenvectors.
- [`svd_s`](svd_s.md): singular values, the analogue for rectangular matrices.
- [`det`](det.md): the determinant, the product of the eigenvalues for a symmetric matrix.
- [`logdet`](logdet.md): the sum of the logarithms of the eigenvalues of a positive-definite matrix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eigh.html"><span class="label">Previous</span><span class="title">eigh</span></a>
  <a class="next-link" href="gen_eigh.html"><span class="label">Next</span><span class="title">gen_eigh</span></a>
</div>
