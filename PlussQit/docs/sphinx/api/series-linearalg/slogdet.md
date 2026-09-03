<!-- pluss-hand-authored -->

# SeriesLinearAlgAccessor::slogdet

[Back to SeriesLinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesLinearAlgAccessor::slogdet() const
Series SeriesLinearAlgProxy::slogdet() const
```

## What It Does

`slogdet()` decomposes each determinant into sign and logarithmic magnitude:

```text
slogdet(A) = [sign(det(A)), log(abs(det(A)))]
```

It returns both values in one `Series`. The final output dimension has length
two:

```text
result[..., 0] = sign
result[..., 1] = log-absolute-determinant
```

This representation retains orientation while keeping determinant magnitude
in a logarithmic range.

## Separating Orientation From Magnitude

A real determinant contains two distinct pieces of information. Its sign says
whether the transformation preserves or reverses orientation. Its magnitude
says how strongly area or volume is scaled. Logging the magnitude makes very
large and very small scales manageable, while storing sign separately avoids
asking the real logarithm to represent a negative number.

The original determinant can be reconstructed conceptually as

```text
det(A) = sign * exp(logabsdet)
```

For singular matrices, the pair is `[0, -inf]`. In the limiting expression,
`exp(-inf)` is zero, and the zero sign clearly marks the loss of invertibility.

## Shape Contract

| Input shape | Output shape | Layout |
| --- | --- | --- |
| `[n, n]` | `[2]` | `[sign, logabsdet]` |
| `[b, n, n]` | `[b, 2]` | one pair per matrix |
| `[..., n, n]` | `[..., 2]` | batch shape followed by pair axis |

Unlike `det()` and `logdet()`, the result is not rank zero for one matrix. The
two matrix dimensions are replaced by a final length-two result dimension.

## Returns

`Series`
    A single `Series` whose last dimension stores sign at index `0` and
    log-absolute-determinant at index `1`.

    For real input, sign is `1`, `-1`, or `0`. The log-magnitude is finite for
    a nonsingular finite matrix and `-inf` for a singular matrix.

## Gradient

The sign output is treated as piecewise constant. The differentiable
log-magnitude output has the same local derivative as `logdet()` away from
singular matrices:

```text
d log(abs(det(A))) / dA = A^-T
```

If the incoming sensitivity along the final result axis is `[g_sign, g_log]`,
only `g_log` contributes:

```text
dL/dA = g_log A^-T
```

This makes `select(..., 1)` the natural scalar objective when differentiating
the log-magnitude of one matrix.

## Examples

Example 1. A positive determinant produces positive sign and its logarithmic
magnitude.

```cpp
auto matrix = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
std::cout << matrix.linearalg.slogdet() << "\n";
```

The determinant is `10`, so the pair is `[1, log(10)]`.

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2.3025851]
```

Example 2. Row exchange changes only the sign component.

```cpp
auto original = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
auto swapped = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});

std::cout << "original=" << original.linearalg.slogdet() << "\n";
std::cout << "swapped=" << swapped.linearalg.slogdet() << "\n";
```

Both determinants have magnitude `10`.

### Output

```text
original=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2.3025851]
swapped=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1, 2.3025851]
```

Example 3. Singular input has zero sign and negative-infinite log-magnitude.

```cpp
auto singular = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
std::cout << singular.linearalg.slogdet() << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, -inf]
```

Example 4. Batched input appends a pair axis after the batch axis.

```cpp
auto batch = pluss::make_series({
    {{1.0, 0.0}, {0.0, 3.0}},
    {{2.0, 1.0}, {1.0, 2.0}},
    {{1.0, 2.0}, {2.0, 4.0}},
});

std::cout << batch.linearalg.slogdet() << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1.0986123],
  [1, 1.0986123],
  [0, -inf]]
```

The first axis still identifies the three input matrices. The final axis now
selects sign or log-magnitude.

Example 5. Select the log-magnitude slot before reverse mode for one matrix.

```cpp
auto matrix = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
matrix.tracks_slope_(true);

matrix.linearalg.slogdet().select(0, 1).reverse();

std::cout << matrix.sensitivity() << "\n";
```

The selected slot is `log(abs(det(A)))`; its derivative is `A^-T`.

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.5, 0],
  [-0, 0.25]]
```

Example 6. Reconstruct a moderate determinant by indexing the two result
slots. This is useful when the ordinary-scale value is genuinely needed.

```cpp
auto matrix = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});
auto signed_log = matrix.linearalg.slogdet();
auto reconstructed = signed_log.select(0, 0).mul(
    signed_log.select(0, 1).exp());

std::cout << reconstructed << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-10
```

## Gotchas And Failure Modes

- `slogdet()` returns one `Series`, not a pair of C++ objects.
- The last output dimension is always the `[sign, logabsdet]` axis. For a
  batched result, select along that final dimension rather than batch dimension
  zero.
- Singular input returns `[0, -inf]`; reconstructing with ordinary arithmetic
  should account for the zero sign explicitly.
- The sign slot does not contribute a reverse sensitivity. The log-magnitude
  derivative is undefined at singular matrices.
- Input must end in square matrix dimensions.

## Summary

`slogdet()` preserves determinant orientation and logarithmic magnitude in one
`[..., 2]` result. Index `0` is sign, index `1` is log-magnitude, and only the
log-magnitude slot contributes the inverse-transpose gradient.

## See Also

- [`SeriesLinearAlgAccessor::det`](det.md): ordinary signed determinant.
- [`SeriesLinearAlgAccessor::logdet`](logdet.md): real log of a positive determinant.
- [`SeriesLinearAlgAccessor::inverse`](inverse.md): inverse-transpose relationship.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qr_r.html">
    <span class="label">Previous</span>
    <span class="title">qr_r</span>
  </a>
  <a class="next-link" href="svd.html">
    <span class="label">Next</span>
    <span class="title">svd</span>
  </a>
</div>
