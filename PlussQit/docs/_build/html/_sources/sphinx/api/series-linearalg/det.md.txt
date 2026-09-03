<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::det

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::det() const
Series SeriesLinearAlgProxy::det() const
```

## What It Does

`det()` computes the determinant of a square matrix, or one determinant for
every square matrix in a batch. For a two-by-two matrix,

```text
A = [[a, b],       det(A) = ad - bc
     [c, d]]
```

The determinant is a signed scale factor. Its magnitude tells how much the
matrix scales area or volume; its sign records orientation. A zero determinant
means that at least one dimension has collapsed, so the matrix is singular.

## Signed Scale And Orientation

Do not read `det()` as a product of the diagonal. That shortcut works for
triangular matrices, but not for a general matrix. The off-diagonal entries
participate because the determinant combines every way rows can contribute
independent directions.

Three facts make the result useful:

- swapping two rows reverses the sign
- multiplying one row by a scalar multiplies the determinant by that scalar
- dependent rows produce zero

For batched input, the last two dimensions are always the matrix dimensions.
Every leading coordinate selects an independent matrix.

## Shape Contract

| Input shape | Interpretation | Output shape |
| --- | --- | --- |
| `[n, n]` | one square matrix | `[]` |
| `[b, n, n]` | `b` square matrices | `[b]` |
| `[..., n, n]` | a batch over all leading dimensions | `[...]` |

The matrix dimensions must be square. `det()` removes exactly the final two
dimensions and preserves every batch dimension.

## Returns

`Series`
    A scalar for one matrix, or a `Series` over the input's batch dimensions.
    Positive, negative, and zero determinants retain their sign.

## Gradient

For an invertible real matrix and scalar objective `L = det(A)`,

```text
dL/dA = det(A) A^-T
```

Equivalently, each entry receives its cofactor. The cofactor form is important
at singular matrices: it defines the determinant's derivative even where an
inverse does not exist. With incoming sensitivity `g`, multiply the expression
by `g`. Batched inputs apply the same rule independently to each matrix.

## Examples

Example 1. A two-by-two determinant includes both diagonal products.

```cpp
auto matrix = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
std::cout << matrix.linearalg.det() << "\n";
```

Here `4*6 - 7*2 = 10`.

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=10
```

Example 2. Swapping the rows reverses orientation without changing magnitude.

```cpp
auto original = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
auto swapped = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});

std::cout << "original=" << original.linearalg.det() << "\n";
std::cout << "swapped=" << swapped.linearalg.det() << "\n";
```

### Output

```text
original=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=10
swapped=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-10
```

Example 3. A batch returns one determinant per matrix. The third matrix has a
second row equal to twice its first, so it is singular.

```cpp
auto batch = pluss::make_series({
    {{1.0, 0.0}, {0.0, 3.0}},
    {{2.0, 1.0}, {1.0, 2.0}},
    {{1.0, 2.0}, {2.0, 4.0}},
});

std::cout << batch.linearalg.det() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 3, 0]
```

Example 4. Reverse mode returns the cofactor matrix.

```cpp
auto matrix = pluss::make_series({{2.0, 0.5}, {0.25, 1.5}});
matrix.tracks_slope_(true);

matrix.linearalg.det().reverse();

std::cout << "value=" << matrix.linearalg.det() << "\n";
std::cout << "gradient=" << matrix.sensitivity() << "\n";
```

For `[[a,b],[c,d]]`, the entrywise derivative is
`[[d,-c],[-b,a]]`.

### Output

```text
value=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.875
gradient=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.5, -0.25],
  [-0.5, 2]]
```

Example 5. Rectangular matrices have no determinant.

```cpp
try {
  auto rectangular = pluss::make_series(
      {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  std::cout << rectangular.linearalg.det() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
subsp::math::linalg::det: expected square matrix
```

## Gotchas And Failure Modes

- The final two dimensions must exist and must have equal length.
- A zero result is meaningful; it identifies a singular matrix and is not an
  exception.
- Determinants can overflow or underflow for large matrices or extreme values.
  Use [`slogdet()`](slogdet.md) when log-magnitude is the quantity the algorithm
  needs.
- A nonzero determinant mathematically implies invertibility, but values very
  close to zero can still indicate a numerically delicate matrix.

## Summary

`det()` reduces each square matrix to its signed scale factor. It removes the
two matrix dimensions, preserves batch dimensions, and returns zero for a
singular matrix.

## See Also

- [`SeriesLinearAlgAccessor::slogdet`](slogdet.md): sign and log-magnitude.
- [`SeriesLinearAlgAccessor::logdet`](logdet.md): real logarithm of a positive determinant.
- [`SeriesLinearAlgAccessor::inverse`](inverse.md): multiplicative inverse of a matrix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cholesky_solve.html">
    <span class="label">Previous</span>
    <span class="title">cholesky_solve</span>
  </a>
  <a class="next-link" href="dot.html">
    <span class="label">Next</span>
    <span class="title">dot</span>
  </a>
</div>
