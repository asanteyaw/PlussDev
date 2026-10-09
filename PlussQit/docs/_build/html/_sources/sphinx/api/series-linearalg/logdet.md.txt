<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::logdet

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::logdet() const
Series SeriesLinearAlgProxy::logdet() const
```

## What It Does

`logdet()` computes the real natural logarithm of each matrix determinant:

```text
logdet(A) = log(det(A))
```

The real logarithm requires a positive determinant. A positive determinant
produces a finite logarithm, a zero determinant produces negative infinity,
and a negative determinant produces `NaN` because its logarithm is outside the
real number domain.

Like `det()`, this method reduces the final two matrix dimensions and preserves
all leading batch dimensions.

## Why Work In Logarithmic Scale

Determinants multiply under matrix multiplication:

```text
det(A B) = det(A) det(B)
```

Their logarithms therefore add whenever the determinants are positive:

```text
logdet(A B) = logdet(A) + logdet(B)
```

This turns a potentially enormous or tiny multiplicative scale into a more
manageable additive quantity. It also explains the domain: `logdet()` retains
the ordinary real-logarithm meaning rather than discarding determinant sign.
When sign is itself part of the result, [`slogdet()`](slogdet.md) represents
both sign and log-magnitude explicitly.

## Shape Contract

| Input shape | Output shape | One output value means |
| --- | --- | --- |
| `[n, n]` | `[]` | `log(det(A))` |
| `[b, n, n]` | `[b]` | one log-determinant per batch member |
| `[..., n, n]` | `[...]` | one value per leading coordinate |

The final two dimensions must be square. They are reduced; leading dimensions
are not broadcast or combined.

## Returns

`Series`
    A scalar or batch-shaped `Series` containing real log-determinants.
    Positive determinants produce their natural logarithm, zero determinants
    produce `-inf`, and negative determinants produce `NaN`.

## Gradient

For an invertible matrix with positive determinant,

```text
d log(det(A)) / dA = A^-T
```

With incoming sensitivity `g`, reverse mode returns `g A^-T`. This follows by
dividing the determinant derivative `det(A) A^-T` by `det(A)`: the potentially
large determinant cancels, leaving the inverse transpose.

The derivative is not finite at a singular matrix, and a negative determinant
lies outside this method's real-valued forward domain.

## Examples

Example 1. Compute the logarithm of a positive determinant.

```cpp
auto matrix = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
std::cout << matrix.linearalg.logdet() << "\n";
```

The determinant is `10`, so the result is `log(10)`.

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.3025851
```

Example 2. Determinant sign controls the real logarithm's result.

```cpp
auto positive = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
auto negative = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});
auto singular = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});

std::cout << "positive=" << positive.linearalg.logdet() << "\n";
std::cout << "negative=" << negative.linearalg.logdet() << "\n";
std::cout << "singular=" << singular.linearalg.logdet() << "\n";
```

The second matrix has determinant `-10`, while the third has determinant zero.

### Output

```text
positive=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.3025851
negative=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
singular=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-inf
```

Example 3. A batch preserves one output dimension for its three matrices.

```cpp
auto batch = pluss::make_series({
    {{1.0, 0.0}, {0.0, 3.0}},
    {{2.0, 1.0}, {1.0, 2.0}},
    {{1.0, 2.0}, {2.0, 4.0}},
});

std::cout << batch.linearalg.logdet() << "\n";
```

The first two determinants are `3`; the final matrix is singular.

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0986123, 1.0986123, -inf]
```

Example 4. The reverse sensitivity is the inverse transpose.

```cpp
auto matrix = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
matrix.tracks_slope_(true);

matrix.linearalg.logdet().reverse();

std::cout << matrix.sensitivity() << "\n";
```

The matrix is diagonal, so its inverse transpose is
`diag(1/2, 1/4)`.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.5, 0],
  [-0, 0.25]]
```

Example 5. The additive rule can replace a product determinant calculation
when both determinants are positive.

```cpp
auto a = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
auto b = pluss::make_series({{4.0, 0.0}, {0.0, 5.0}});

std::cout << "product=" << a.linearalg.mm(b).linearalg.logdet() << "\n";
std::cout << "sum=" << a.linearalg.logdet().add(b.linearalg.logdet()) << "\n";
```

Both expressions equal `log(120)`.

### Output

```text
product=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.7874917
sum=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[4.7874917]
```

## Gotchas And Failure Modes

- `logdet()` means `log(det(A))`, not `log(abs(det(A)))`.
- Negative determinants produce `NaN`; use `slogdet()` when the sign and
  logarithm of the magnitude are both meaningful to the algorithm.
- Singular matrices produce `-inf`, and their derivative is not finite.
- Input must end in two equal matrix dimensions.

## Summary

`logdet()` maps each positive determinant into the real logarithmic domain,
turning multiplicative matrix scale into an additive quantity. Its output has
the batch shape, and its gradient is the inverse transpose.

## See Also

- [`Series::LinearAlgAccessor::slogdet`](slogdet.md): signed log-magnitude.
- [`Series::LinearAlgAccessor::det`](det.md): signed determinant value.
- [`Series::LinearAlgAccessor::inverse`](inverse.md): source of the gradient formula.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inverse.html">
    <span class="label">Previous</span>
    <span class="title">inverse</span>
  </a>
  <a class="next-link" href="lu_solve.html">
    <span class="label">Next</span>
    <span class="title">lu_solve</span>
  </a>
</div>
