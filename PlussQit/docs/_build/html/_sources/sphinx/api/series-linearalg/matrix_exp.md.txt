<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::matrix_exp

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::matrix_exp() const
Series SeriesLinearAlgProxy::matrix_exp() const
```

## What It Does

`matrix_exp()` computes the matrix exponential

```text
exp(A) = I + A + A^2/2! + A^3/3! + ...
```

Powers in this series are matrix powers. This is not the same operation as
applying scalar `exp()` to every entry. The identity term supplies ones on the
diagonal, and every higher term mixes entries through matrix multiplication.

## From Rates To Finite Transformations

The matrix exponential turns an infinitesimal linear change into a finite
transformation. If `A` describes a constant rate of change, `exp(A)` describes
the accumulated transformation over one unit of time. Scaling first gives
`exp(tA)`, the transformation after duration `t`.

Several special cases make the definition concrete:

- `exp(0) = I`
- a diagonal matrix exponentiates its diagonal entries
- if `A^2 = 0`, every term after `A` vanishes and `exp(A) = I + A`
- `exp(A + B) = exp(A)exp(B)` only when `A` and `B` commute

The implementation scales the matrix into a numerically manageable range,
evaluates its exponential series, then repeatedly squares the result. This
scaling-and-squaring step is what permits large matrix norms without relying on
an unstable unscaled series.

## Shape Contract

| Input shape | Output shape |
| --- | --- |
| `[d, d]` | `[d, d]` |
| `[b, d, d]` | `[b, d, d]` |
| `[..., d, d]` | `[..., d, d]` |

Every final two-dimensional slice must be square. Batch dimensions and matrix
dimensions are preserved.

## Returns

`Series`
    The matrix exponential of each input matrix, with the same shape as the
    input.

## Gradient

For matrices, the derivative cannot generally be written as
`exp(A) * dA`, because `A` and its perturbation may not commute. The derivative
is the Fréchet derivative

```text
d exp(A)[E] = integral from 0 to 1 of exp((1-t)A) E exp(tA) dt
```

Reverse mode applies the adjoint of that linear map to the incoming sensitivity
`G`. Each batch member is differentiated independently. The implementation
evaluates this rule through a block-matrix exponential, preserving the full
off-diagonal coupling.

## Examples

Example 1. The zero matrix exponentiates to identity.

```cpp
auto zero = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
std::cout << zero.linearalg.matrix_exp() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 1]]
```

Example 2. A nilpotent matrix makes the infinite series terminate exactly.

```cpp
auto nilpotent = pluss::make_series({{0.0, 1.0}, {0.0, 0.0}});
std::cout << nilpotent.linearalg.matrix_exp() << "\n";
```

Here `A^2 = 0`, so `exp(A) = I + A`.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [0, 1]]
```

Example 3. A diagonal matrix reduces to familiar scalar exponentials along its
diagonal.

```cpp
auto diagonal = pluss::make_series({
    {0.0, 0.0},
    {0.0, 0.6931471805599453},
});

std::cout << diagonal.linearalg.matrix_exp() << "\n";
```

The second diagonal value is `log(2)`.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 2]]
```

Example 4. Scaling and squaring handles a large positive and negative diagonal
pair without truncating the unscaled series.

```cpp
auto matrix = pluss::make_series({{20.0, 0.0}, {0.0, -20.0}});
std::cout << matrix.linearalg.matrix_exp() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4.851652e+08, 0],
  [0, 2.0611536e-09]]
```

Example 5. A batch applies the operation independently to each final matrix.

```cpp
auto batch = pluss::make_series({
    {{0.0, 0.0}, {0.0, 0.0}},
    {{0.0, 1.0}, {0.0, 0.0}},
});

std::cout << batch.linearalg.matrix_exp() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 0],
  [0, 1]],
  [[1, 1],
  [0, 1]]]
```

Example 6. Reverse mode preserves coupling between different matrix entries.

```cpp
auto matrix = pluss::make_series({
    {0.0, 0.0},
    {0.0, 0.6931471805599453},
});
matrix.tracks_slope_(true);

matrix.linearalg.matrix_exp().sum().reverse();

std::cout << matrix.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1.442695],
  [1.442695, 2]]
```

The off-diagonal value is the divided difference
`(exp(log(2)) - exp(0)) / (log(2) - 0) = 1/log(2)`. It demonstrates why an
elementwise exponential derivative would be incorrect.

## Gotchas And Failure Modes

- `matrix_exp()` is different from [`Series::exp`](../series/exp.md), which
  exponentiates entries independently.
- The final two dimensions must be square.
- The identity `exp(A+B)=exp(A)exp(B)` requires commuting matrices; it is not a
  general distribution rule.
- Very large positive growth directions can still overflow the output's dtype,
  because the mathematical exponential itself may be outside its finite range.
- Complex reverse mode is not currently defined for this method.

## Summary

`matrix_exp()` sums matrix powers to convert a linear rate into a finite
transformation. It preserves matrix and batch shapes, uses scaling and squaring
for numerical range, and differentiates through the full matrix coupling.

## See Also

- [`Series::LinearAlgAccessor::matrix_power`](matrix_power.md): integer matrix powers.
- [`Series::exp`](../series/exp.md): elementwise exponential.
- [`Series::LinearAlgAccessor::matmul`](matmul.md): matrix composition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="matrix_power.html">
    <span class="label">Next</span>
    <span class="title">matrix_power</span>
  </a>
</div>
