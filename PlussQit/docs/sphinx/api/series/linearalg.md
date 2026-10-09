<!-- pluss-hand-authored -->

# Series::linearalg

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::linearalg -> Series::LinearAlgAccessor

Series::linearalg() -> SeriesLinearAlgProxy
Series::linearalg(const Series& other) -> SeriesLinearAlgProxy
```

## What It Does

`linearalg` is a namespace-like accessor for vector and matrix operations. It
does not compute a value by itself. It selects the linear-algebra method family
and keeps the owning `Series` as the left operand.

The distinction matters because these methods do not use one common
elementwise shape rule. A matrix product contracts dimensions, a batched
matrix product broadcasts only its batch dimensions, and a dot product
requires two equally long vectors and reduces them to a scalar. The accessor
gives those related rules one visible home.

## How The Accessor Organizes Matrix Work

Think of `linearalg` as a door, not an operation. The owning `Series` stays on
the left side of every binary operation; the accessor chooses a family in
which dimensions have algebraic roles:

- a vector dimension is contracted by an inner product
- a matrix's last two dimensions mean rows and columns
- leading dimensions in `matmul()` identify batches of independent products
- strict methods such as `mm()`, `mv()`, and `dot()` reject ranks that would
  change the intended operation

That distinction is why `linearalg` deserves its own reference. Elementwise
`mul()` asks which positions broadcast together. Linear-algebra products ask
which dimensions contract and which survive in the output.

## Calling Styles

The dot-style form passes peers to individual methods:

```cpp
auto result = left.linearalg.matmul(right);
```

The proxy style binds a peer first:

```cpp
auto result = left.linearalg(right).matmul();
```

Both calls treat `left` as the left operand and `right` as the right operand.
Binding a peer does not reverse the order.

Unary methods do not need a peer:

```cpp
auto determinant = matrix.linearalg.det();
auto same_result = matrix.linearalg().det();
```

The complete accessor surface is documented: products, norms, determinants,
matrix transformations, factorizations, and structured or general solves.

Use the [Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)
as the landing page for this family. Each accessor method is documented there
on its own page:

- rank-polymorphic multiplication: [`matmul()`](../series-linearalg/matmul.md)
- strict matrix products: [`mm()`](../series-linearalg/mm.md) and
  [`mv()`](../series-linearalg/mv.md)
- vector inner products: [`dot()`](../series-linearalg/dot.md) and
  [`vdot()`](../series-linearalg/vdot.md)
- determinant values: [`det()`](../series-linearalg/det.md),
  [`logdet()`](../series-linearalg/logdet.md), and
  [`slogdet()`](../series-linearalg/slogdet.md)
- matrix inversion: [`inverse()`](../series-linearalg/inverse.md)
- integer and continuous matrix transforms:
  [`matrix_power()`](../series-linearalg/matrix_power.md) and
  [`matrix_exp()`](../series-linearalg/matrix_exp.md)
- rectangular and rank-deficient reversal:
  [`pinverse()`](../series-linearalg/pinverse.md)
- global magnitude: [`norm()`](../series-linearalg/norm.md)
- positive-definite factorization: [`cholesky()`](../series-linearalg/cholesky.md)
- orthogonal-triangular factorization:
  [`qr_q()`](../series-linearalg/qr_q.md) and [`qr_r()`](../series-linearalg/qr_r.md)
- singular-value decomposition: [`svd()`](../series-linearalg/svd.md),
  [`svd_u()`](../series-linearalg/svd_u.md),
  [`svd_s()`](../series-linearalg/svd_s.md), and
  [`svd_vh()`](../series-linearalg/svd_vh.md)
- structured and general systems:
  [`triangular_solve()`](../series-linearalg/triangular_solve.md),
  [`cholesky_solve()`](../series-linearalg/cholesky_solve.md),
  [`cholesky_inverse()`](../series-linearalg/cholesky_inverse.md), and
  [`lu_solve()`](../series-linearalg/lu_solve.md)

## Returns

`Series::LinearAlgAccessor` or `SeriesLinearAlgProxy`
    A lightweight method-selection object bound to the owning `Series`, and
    optionally to a peer `Series`.

    Accessing `linearalg` does not copy numeric data and does not mutate the
    owner.

## Example

The two binary call styles are equivalent.

```cpp
auto left = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto right = pluss::make_series({{2.0, 0.0}, {1.0, 2.0}});

std::cout << "peer_argument=" << left.linearalg.mm(right) << "\n";
std::cout << "peer_bound=" << left.linearalg(right).mm() << "\n";
```

### Output

```text
peer_argument=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [10, 8]]
peer_bound=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [10, 8]]
```

The accessor is bound to `left`, so both forms calculate `left * right`. This
is observable with non-commuting matrices: changing which object owns the
accessor can change the result.

## Gotchas And Failure Modes

- `linearalg` is an accessor, not a materialized result.
- A peer-bound zero-argument binary method requires a peer to have been passed
  to `linearalg(...)` first.
- The owner remains the left operand. This is important for non-commutative
  methods such as matrix multiplication and linear solves.
- Individual methods still enforce their own shape, dtype, platform, and
  decomposition requirements.

## See Also

- [Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)
- [`Series::LinearAlgAccessor::matmul`](../series-linearalg/matmul.md)
- [`Series::LinearAlgAccessor::dot`](../series-linearalg/dot.md)
- [`Series::LinearAlgAccessor::det`](../series-linearalg/det.md)
- [`Series::LinearAlgAccessor::inverse`](../series-linearalg/inverse.md)
- [`Series::LinearAlgAccessor::matrix_exp`](../series-linearalg/matrix_exp.md)
- [`Series::LinearAlgAccessor::pinverse`](../series-linearalg/pinverse.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lgamma.html">
    <span class="label">Previous</span>
    <span class="title">lgamma</span>
  </a>
  <a class="next-link" href="../../series_linearalg_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series::LinearAlgAccessor</span>
  </a>
</div>
