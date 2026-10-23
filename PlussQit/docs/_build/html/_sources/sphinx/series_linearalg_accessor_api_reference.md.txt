<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

This page is the landing page for the linear-algebra workflow entered through
[`Series::linearalg`](api/series/linearalg.md) and continued through
`Series::LinearAlgAccessor`. The accessor groups operations whose shapes are
governed by vector, matrix, and batched-matrix rules rather than ordinary
elementwise broadcasting.

## Using This Reference

- Use the method rail to open one accessor method at a time.
- Each method page owns its signatures, shape contract, gradient model,
  verified examples, failures, and previous/next navigation.
- Start at [`Series::linearalg`](api/series/linearalg.md) when you need the
  accessor calling model; start here when you already know which operation you
  need.

## Read This First

The accessor supports two equivalent call styles:

```cpp
auto out1 = left.linearalg.matmul(right);
auto out2 = left.linearalg(right).matmul();
```

The first style passes the right operand to the method. The second binds it
when the accessor proxy is created and then calls a zero-argument method. Use
the first style in ordinary code unless binding a peer makes a longer chain
easier to read.

Several operations also have a direct root spelling such as
`left.matmul(right)`. The accessor spelling is useful when you want the code to
announce that an operation follows linear-algebra shape rules.

## Method Pages

### Norms

- [`norm`](api/series-linearalg/norm.md): global Euclidean or Frobenius
  magnitude

### Factorizations

- [`cholesky`](api/series-linearalg/cholesky.md): triangular factorization of
  symmetric positive-definite matrices
- [`qr_q`](api/series-linearalg/qr_q.md) and
  [`qr_r`](api/series-linearalg/qr_r.md): orthonormal-basis and triangular
  factors of a rectangular matrix
- [`svd`](api/series-linearalg/svd.md): obtain singular vectors and singular
  values together
- [`eigh`](api/series-linearalg/eigh.md) and
  [`eigvalsh`](api/series-linearalg/eigvalsh.md): eigenvalues and orthonormal
  eigenvectors, or eigenvalues alone, of a symmetric or Hermitian matrix
- [`gen_eigh`](api/series-linearalg/gen_eigh.md): lowest positive generalized
  eigenpairs from matrix-free Hermitian operator and inverse actions
- [`svd_u`](api/series-linearalg/svd_u.md),
  [`svd_s`](api/series-linearalg/svd_s.md), and
  [`svd_vh`](api/series-linearalg/svd_vh.md): request one SVD component

### Determinants And Inversion

- [`det`](api/series-linearalg/det.md): signed scale factor of each square
  matrix
- [`inverse`](api/series-linearalg/inverse.md): multiplicative inverse of each
  nonsingular square matrix
- [`logdet`](api/series-linearalg/logdet.md): real logarithm of each positive
  determinant
- [`slogdet`](api/series-linearalg/slogdet.md): determinant sign and logarithmic
  magnitude in one result

### Products

- [`matmul`](api/series-linearalg/matmul.md): rank-polymorphic vector, matrix,
  and batched matrix multiplication
- [`mm`](api/series-linearalg/mm.md): matrix-matrix multiplication with a strict
  two-dimensional contract
- [`mv`](api/series-linearalg/mv.md): matrix-vector multiplication
- [`dot`](api/series-linearalg/dot.md): non-conjugating inner product of two
  vectors
- [`vdot`](api/series-linearalg/vdot.md): vector inner product that
  conjugates the left operand

### Matrix Transformations

- [`matrix_power`](api/series-linearalg/matrix_power.md): compose a square
  transformation an integer number of times
- [`matrix_exp`](api/series-linearalg/matrix_exp.md): convert a linear rate into
  a finite transformation through the matrix exponential
- [`pinverse`](api/series-linearalg/pinverse.md): SVD-defined pseudoinverse for
  square, rectangular, and rank-deficient matrices

### Linear Solves

- [`triangular_solve`](api/series-linearalg/triangular_solve.md): forward or
  backward substitution through a triangular coefficient matrix
- [`cholesky_solve`](api/series-linearalg/cholesky_solve.md): solve an SPD
  system from a reusable Cholesky factor
- [`cholesky_inverse`](api/series-linearalg/cholesky_inverse.md): construct an
  SPD inverse from a Cholesky factor
- [`lu_solve`](api/series-linearalg/lu_solve.md): solve a general nonsingular
  square system

The handwritten reference now covers the complete `Series::LinearAlgAccessor`
surface.

## Choosing A Product

Use `matmul()` when the code intentionally accepts more than one rank pattern:
vectors contract, matrices multiply, and leading batch dimensions broadcast.
Use `mm()` or `mv()` when rank is part of the contract and an accidental vector
or matrix should be rejected. Use `dot()` for two one-dimensional vectors.
Use `vdot()` instead of `dot()` when the left input may be complex and the
Hermitian inner product is intended.

## Choosing A Determinant Operation

Use `det()` when the signed determinant itself is the required value. Use
`logdet()` when the determinant is known to be positive and the algorithm works
in the real logarithmic domain. Use `slogdet()` when sign must be retained or
the magnitude may span a large numeric range. Use `inverse()` when the inverse
matrix itself is needed; singular matrices have a zero determinant and no
inverse.

## Choosing A Matrix Transformation

Use `matrix_power()` for an integer number of repeated applications, including
zero and negative powers. Use `matrix_exp()` when a matrix represents a
continuous linear rate whose finite transformation is required. Use
`pinverse()` when a rectangular or rank-deficient map needs a least-squares
reversal rather than a strict square inverse.

```{toctree}
:hidden:
:maxdepth: 1

api/series-linearalg/cholesky
api/series-linearalg/cholesky_inverse
api/series-linearalg/cholesky_solve
api/series-linearalg/det
api/series-linearalg/dot
api/series-linearalg/eigh
api/series-linearalg/eigvalsh
api/series-linearalg/gen_eigh
api/series-linearalg/inverse
api/series-linearalg/logdet
api/series-linearalg/lu_solve
api/series-linearalg/matmul
api/series-linearalg/matrix_exp
api/series-linearalg/matrix_power
api/series-linearalg/mm
api/series-linearalg/mv
api/series-linearalg/norm
api/series-linearalg/pinverse
api/series-linearalg/qr_q
api/series-linearalg/qr_r
api/series-linearalg/slogdet
api/series-linearalg/svd
api/series-linearalg/svd_s
api/series-linearalg/svd_u
api/series-linearalg/svd_vh
api/series-linearalg/triangular_solve
api/series-linearalg/vdot
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/linearalg.html">
    <span class="label">Previous</span>
    <span class="title">linearalg</span>
  </a>
  <a class="next-link" href="api/series-linearalg/cholesky.html">
    <span class="label">Next</span>
    <span class="title">cholesky</span>
  </a>
</div>
