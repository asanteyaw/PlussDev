<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::matrix_power

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::matrix_power(std::int64_t n) const
Series SeriesLinearAlgProxy::matrix_power(std::int64_t n) const
```

## What It Does

`matrix_power(n)` raises every square matrix to the integer power `n` using
matrix multiplication:

```text
A^3 = A A A
```

It is not an elementwise power. Rows and columns contract at every
multiplication, so off-diagonal entries can influence the entire result.

The exponent determines three distinct operations:

| `n` | Result |
| --- | --- |
| positive | repeated matrix multiplication |
| zero | the identity matrix |
| negative | the corresponding positive power of `A^-1` |

Negative powers therefore require every input matrix to be invertible.

## Repeated Composition

A matrix often represents one step of a linear transformation. `A^n` applies
that same step `n` times. A shear accumulates displacement, a diagonal matrix
repeatedly scales its coordinate directions, and a transition matrix advances
the represented system by several steps.

Order still matters. Matrix multiplication is generally noncommutative, so the
derivative of `A^n` cannot be simplified to the elementwise-looking expression
`n A^(n-1)` unless all relevant matrices commute. The implementation uses
exponentiation by squaring for the forward value, so large integer powers do
not require one multiplication per step.

## Parameter

**n** (`std::int64_t`)
    The integer exponent. Zero is valid. Negative values first invert each
    matrix and fail if any matrix is singular.

## Shape Contract

| Input shape | Output shape |
| --- | --- |
| `[d, d]` | `[d, d]` |
| `[b, d, d]` | `[b, d, d]` |
| `[..., d, d]` | `[..., d, d]` |

The final two dimensions must be square. All leading batch dimensions and both
matrix dimensions are preserved.

## Returns

`Series`
    A matrix or batch of matrices with the same shape as the input.

## Gradient

For a positive exponent `n` and incoming matrix sensitivity `G`, reverse mode
uses the full noncommutative sum

```text
dL/dA = sum from k=0 to n-1 of (A^k)^T G (A^(n-1-k))^T
```

For `n = 0`, the output is constant and the gradient is zero. Negative powers
differentiate the positive power of `A^-1` and then apply the inverse rule.
These rules are evaluated independently for every matrix in a batch.

## Examples

Example 1. Repeatedly applying a shear accumulates its off-diagonal term.

```cpp
auto shear = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});
std::cout << shear.linearalg.matrix_power(3) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 6],
  [0, 1]]
```

Example 2. Power zero returns an identity matrix, not an elementwise matrix of
ones.

```cpp
auto matrix = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});
std::cout << matrix.linearalg.matrix_power(0) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 1]]
```

Example 3. A negative exponent applies the inverse transformation.

```cpp
auto shear = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});
std::cout << shear.linearalg.matrix_power(-1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, -2],
  [0, 1]]
```

The inverse shear cancels the original displacement.

Example 4. Batched matrices are powered independently.

```cpp
auto batch = pluss::make_series({
    {{1.0, 1.0}, {0.0, 1.0}},
    {{2.0, 0.0}, {0.0, 3.0}},
});

std::cout << batch.linearalg.matrix_power(2) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 2],
  [0, 1]],
  [[4, 0],
  [0, 9]]]
```

Example 5. Reverse mode includes every possible position at which the input
matrix can vary inside `A A A`.

```cpp
auto matrix = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
matrix.tracks_slope_(true);

matrix.linearalg.matrix_power(3).sum().reverse();

std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[12, 19],
  [19, 27]]
```

The off-diagonal sensitivities are nonzero even though the input matrix is
diagonal. Perturbing an off-diagonal entry creates paths that mix the two
diagonal scales during multiplication.

Example 6. Rectangular matrices are rejected because repeated multiplication
would not preserve one matrix shape.

```cpp
try {
  auto rectangular = pluss::make_series(
      {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  std::cout << rectangular.linearalg.matrix_power(2) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
subsp::math::linalg::matrix_power: expected square matrix
```

## Gotchas And Failure Modes

- `matrix_power()` is not [`Series::pow`](../series/pow.md); it contracts matrix
  dimensions instead of exponentiating individual elements.
- Power zero returns an identity matrix even when the input itself is singular.
- A negative power fails for singular matrices because it requires `A^-1`.
- Large positive powers can overflow when the transformation has expanding
  directions; large negative powers can magnify near-singularity.
- Complex forward values are supported only where the selected backend's
  matrix operations support them; complex reverse mode is not currently
  defined for this method.

## Summary

`matrix_power(n)` composes a square transformation an integer number of times.
It preserves shape and batches, treats zero as identity, and interprets
negative exponents through matrix inversion.

## See Also

- [`Series::LinearAlgAccessor::matrix_exp`](matrix_exp.md): exponential series of a matrix.
- [`Series::LinearAlgAccessor::inverse`](inverse.md): the `n = -1` case.
- [`Series::LinearAlgAccessor::matmul`](matmul.md): one matrix-composition step.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matrix_exp.html">
    <span class="label">Previous</span>
    <span class="title">matrix_exp</span>
  </a>
  <a class="next-link" href="mm.html">
    <span class="label">Next</span>
    <span class="title">mm</span>
  </a>
</div>
