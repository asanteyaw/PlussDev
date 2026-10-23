<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::eigh

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
EigenDecomposition Series::LinearAlgAccessor::eigh(bool upper = false) const
EigenDecomposition SeriesLinearAlgProxy::eigh(bool upper = false) const
```

## What It Does

`eigh()` decomposes a symmetric (real) or Hermitian (complex) matrix into the
directions it only stretches, and the stretch factors:

```text
A = V diag(L) V^H
```

The returned `EigenDecomposition` has two public `Series` fields:

- `.values`: the eigenvalues `L`, **real** and in ascending order
- `.vectors`: the eigenvectors, one per **column**, orthonormal

`V^H` is the conjugate transpose of `V`; for a real matrix it is the transpose.
Multiplying `A` by column `j` of `V` gives that column scaled by `L[j]`:
`A v_j = L[j] v_j`.

The `h` in the name is the promise that the matrix is symmetric or Hermitian.
That promise is what makes the result well behaved: the eigenvalues are always
real, the eigenvectors are always orthogonal, and the computation is stable.
The method does not check the promise. It reads **one triangle** of each matrix
(see `upper`) and treats the other triangle as its mirror, so a matrix that is
not symmetric is silently replaced by the symmetric matrix defined by the
triangle that was read (Example 3).

Use `eigh()` when both factors are needed: principal axes of a covariance
matrix, a basis in which a quadratic form is diagonal, a matrix function such as
a square root or exponential built as `V f(L) V^H`. Use
[`eigvalsh`](eigvalsh.md) when only the eigenvalues are consumed.

## Calling Styles And The Two Signature Types

```cpp
auto direct = matrix.linearalg.eigh();     // Series::LinearAlgAccessor
auto proxied = matrix.linearalg().eigh();  // SeriesLinearAlgProxy
```

Both expressions decompose `matrix` identically. Eigendecomposition is unary,
so the peer-bound form `matrix.linearalg(other)` has nothing to bind; use the
direct form.

## Parameter

**upper** (`bool`, default `false`)
    Selects the triangle that is read. `false` reads the lower triangle
    (including the diagonal), `true` the upper triangle. The other triangle is
    never read. For an exactly symmetric or Hermitian matrix both choices give
    the same answer.

For a complex matrix, the imaginary part of each diagonal entry is ignored: a
Hermitian matrix has a real diagonal, and the method behaves as if it did.

## Returns

`EigenDecomposition`
    A value object containing `.values` and `.vectors`. It is not itself a
    `Series`; select a field before applying `Series` methods.

For input `[..., n, n]`:

| field | shape | dtype |
| --- | --- | --- |
| `.values` | `[..., n]` | `Float64` for `Float64` or `Complex128` input, `Float32` for `Float32` or `Complex64` input |
| `.vectors` | `[..., n, n]` | the dtype of the input |

Leading dimensions are batch dimensions; every matrix is decomposed
independently. The input must be floating point or complex, at least
two-dimensional, square, and not empty. Integer input is rejected: convert it
first.

Each eigenvector is determined only up to a sign (real case) or a unit complex
factor (complex case): `v` and `-v` are equally valid. The signs and phases
that come back are whatever the solver produced, so compare eigenvectors
through quantities that do not depend on them, such as `A v = L v`, the
projector `v v^H`, or the squared magnitude of an entry. When two eigenvalues
coincide, the whole eigenspace is determined but no particular basis of it is.

## Platforms

The solver runs on the CPU. A matrix on another platform is moved to the CPU,
decomposed there, and the results are moved back, which requires that
platform's CPU solver bridge to be enabled; otherwise the call throws.

## Gradient

Reverse mode is supported for real and complex input, batched or not. The
sensitivity that reaches the matrix is itself symmetric (Hermitian), whichever
triangle was read.

- A loss that depends only on `.values` always has a well-defined gradient.
  For one eigenvalue it is the projector onto its eigenvector, `v v^H`.
- A loss that depends on `.vectors` needs distinct eigenvalues, because the
  derivative of an eigenvector divides by the gap to the other eigenvalues.
  The call throws if the loss depends on the basis of a repeated eigenvalue.
- For a complex matrix the loss must also be unchanged when any eigenvector is
  multiplied by a unit complex factor. A loss such as the squared magnitude of
  an entry is; the real part of an entry is not, and the call throws.

## Examples

Example 1. The two factors for a small symmetric matrix. The eigenvalues are
ascending, and the columns of the eigenvector matrix satisfy `A v = L v`.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}});
auto parts = matrix.linearalg.eigh();

pluss::series::print(std::cout, parts.values);
std::cout << "\n";
auto residual = matrix.linearalg.mm(parts.vectors)
                    .sub(parts.vectors.mul(parts.values.unsqueeze(0)));
std::cout << std::boolalpha << "A v = L v for every column: "
          << (residual.abs().max().item<double>() < 1e-14) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 3.0000]
A v = L v for every column: true
```

The `unsqueeze(0)` turns the eigenvalue vector into a row so that it scales
column `j` by `L[j]`.

Example 2. Rebuild the matrix, and check that the eigenvectors are orthonormal.

```cpp
auto matrix = pluss::make_series({{4.0, 1.0, 0.0}, {1.0, 3.0, 1.0}, {0.0, 1.0, 2.0}});
auto parts = matrix.linearalg.eigh();
auto vt = parts.vectors.transpose(0, 1);

auto rebuilt = parts.vectors.linearalg.mm(parts.values.diag_embed()).linearalg.mm(vt);
auto gram = vt.linearalg.mm(parts.vectors);
std::cout << std::boolalpha
          << "V diag(L) V^T equals A: " << (rebuilt.sub(matrix).abs().max().item<double>() < 1e-14) << "\n"
          << "V^T V is the identity: "
          << (gram.sub(pluss::series::eye(3)).abs().max().item<double>() < 1e-14) << "\n";
pluss::series::print(std::cout, parts.values);
```

### Output

```text
V diag(L) V^T equals A: true
V^T V is the identity: true
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.2679, 3.0000, 4.7321]
```

Example 3. Only one triangle is read. The matrix below is not symmetric: its
lower-left entry is 1 and its upper-right entry is 100. The default reads the
lower triangle, so the matrix is treated as `[[2, 1], [1, 3]]`; `upper = true`
reads the upper triangle and sees `[[2, 100], [100, 3]]`.

```cpp
auto matrix = pluss::make_series({{2.0, 100.0}, {1.0, 3.0}});
pluss::series::print(std::cout, matrix.linearalg.eigh().values);
std::cout << "\n";
pluss::series::print(std::cout, matrix.linearalg.eigh(true).values);
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.3820, 3.6180]
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-97.5012, 102.5012]
```

Neither result is an eigenvalue of the matrix as written; both are eigenvalues
of the symmetric matrix that the chosen triangle defines. Pass a matrix that
really is symmetric, or symmetrize it first (`0.5 * (A + A^T)`).

Example 4. A complex Hermitian matrix. The eigenvalues are real, the
eigenvectors are complex, and `A v = L v` holds in complex arithmetic.

```cpp
using C = std::complex<double>;
auto matrix = pluss::make_series(
    std::vector<C>{{2, 0}, {1, -1}, {1, 1}, {3, 0}}, {2, 2});
auto parts = matrix.linearalg.eigh();

pluss::series::print(std::cout, parts.values);
std::cout << "\n" << std::boolalpha
          << "values are real: " << !parts.values.is_complex() << "\n"
          << "vectors are complex: " << parts.vectors.is_complex() << "\n";
auto residual = matrix.linearalg.mm(parts.vectors)
                    .sub(parts.vectors.mul(parts.values.unsqueeze(0)));
std::cout << "A v = L v for every column: "
          << (residual.abs().max().item<double>() < 1e-14) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 4.0000]
values are real: true
vectors are complex: true
A v = L v for every column: true
```

Example 5. Batches. Leading dimensions are batch dimensions; each matrix is
decomposed on its own.

```cpp
auto stack = pluss::make_series({{{2.0, 0.0}, {0.0, 5.0}},
                                 {{1.0, 2.0}, {2.0, 1.0}},
                                 {{4.0, 0.0}, {0.0, 4.0}}});
auto parts = stack.linearalg.eigh();
std::cout << "values shape: " << parts.values.size(0) << " x " << parts.values.size(1) << "\n";
std::cout << "vectors shape: " << parts.vectors.size(0) << " x " << parts.vectors.size(1)
          << " x " << parts.vectors.size(2) << "\n";
pluss::series::print(std::cout, parts.values);
```

### Output

```text
values shape: 3 x 2
vectors shape: 3 x 2 x 2
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  2.0000 5.0000 ⎤
⎢ -1.0000 3.0000 ⎥
⎣  4.0000 4.0000 ⎦
```

The last matrix has a repeated eigenvalue: any orthonormal pair is a valid
eigenvector matrix there.

Example 6. Single precision stays in single precision.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}}).to(pluss::ScalarType::Float32);
auto parts = matrix.linearalg.eigh();
std::cout << std::boolalpha
          << "values are Float32: " << (parts.values.scalar_type() == pluss::ScalarType::Float32) << "\n"
          << "vectors are Float32: " << (parts.vectors.scalar_type() == pluss::ScalarType::Float32) << "\n";
```

### Output

```text
values are Float32: true
vectors are Float32: true
```

Example 7. Reverse mode through the eigenvalues. The gradient of the largest
eigenvalue of `[[2, 1], [1, 2]]` is the projector `v v^T` onto its eigenvector
`(1, 1) / sqrt(2)`.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 2.0}});
matrix.tracks_slope_(true);
auto parts = matrix.linearalg.eigh();
parts.values.select(0, 1).reverse();
pluss::series::print(std::cout, matrix.sensitivity());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.5000 0.5000 ⎤
⎣ 0.5000 0.5000 ⎦
```

Example 8. Reverse mode through an eigenvector. The squared first component of
the top eigenvector does not depend on the sign of the eigenvector, so its
gradient is well defined.

```cpp
auto matrix = pluss::make_series({{2.0, 1.0}, {1.0, 3.0}});
matrix.tracks_slope_(true);
auto parts = matrix.linearalg.eigh();
auto top = parts.vectors.select(1, 1);
top.select(0, 0).mul(top.select(0, 0)).reverse();
pluss::series::print(std::cout, matrix.sensitivity());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.1789  0.0894 ⎤
⎣ 0.0894 -0.1789 ⎦
```

The gradient is symmetric, as it must be for a symmetric input.

Example 9. Invalid input and ill-defined gradients are rejected.

```cpp
auto first_line = [](const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
};
try {
  pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).linearalg.eigh();
} catch (const std::exception& e) { first_line(e); }
try {
  pluss::make_series({{1, 2}, {2, 1}}).linearalg.eigh();
} catch (const std::exception& e) { first_line(e); }

using C = std::complex<double>;
auto hermitian = pluss::make_series(std::vector<C>{{2, 0}, {1, -1}, {1, 1}, {3, 0}}, {2, 2});
hermitian.tracks_slope_(true);
try {
  hermitian.linearalg.eigh().vectors.real().sum().reverse();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << (message.find("arbitrary phase") != std::string::npos
                    ? "a loss that depends on the eigenvector phase is rejected"
                    : message) << "\n";
}
```

### Output

```text
subsp::math::linalg::eigh: expected shape [..., n, n]
subsp::math::linalg::eigh: expected floating or complex values
a loss that depends on the eigenvector phase is rejected
```

## Gotchas And Failure Modes

- **Symmetry is assumed, not checked.** Only the chosen triangle is read; an
  asymmetric matrix is silently replaced by its symmetric counterpart.
- **Do not compare eigenvectors entry by entry.** Signs and phases are arbitrary.
  Compare `A v = L v`, projectors, or squared magnitudes.
- **Repeated eigenvalues give arbitrary bases.** The eigenspace is unique, its
  basis is not; gradients that depend on the basis throw.
- **Eigenvectors are columns**, not rows (unlike the rows of `svd().vh`).
- **Eigenvalues are ascending**, the opposite of the singular values of
  [`svd`](svd.md).
- **Input must be floating point or complex.** Integer input is rejected.
- **The diagonal of a complex input is taken as real.**

## Summary

`eigh()` returns the real, ascending eigenvalues and the orthonormal eigenvector
columns of a symmetric or Hermitian matrix, batched over leading dimensions,
in the precision of the input. It reads one triangle, selected by `upper`, and is
differentiable for losses that do not depend on eigenvector sign or phase.

## See Also

- [`eigvalsh`](eigvalsh.md): the eigenvalues alone.
- [`svd`](svd.md): the analogous decomposition for any rectangular matrix.
- [`cholesky`](cholesky.md): a triangular factorization of a positive-definite matrix.
- [`matrix_exp`](matrix_exp.md): a matrix function that `eigh` can also build as `V exp(L) V^H` for symmetric input.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dot.html"><span class="label">Previous</span><span class="title">dot</span></a>
  <a class="next-link" href="eigvalsh.html"><span class="label">Next</span><span class="title">eigvalsh</span></a>
</div>
