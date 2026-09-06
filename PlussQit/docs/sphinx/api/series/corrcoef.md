<!-- pluss-hand-authored -->

# Series::corrcoef

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::corrcoef() const
```

## What It Does

Builds the **Pearson correlation matrix** of a set of variables.

Correlation is covariance with the scale removed. Two variables can have a large
covariance simply because they are measured in large units; dividing by the
product of their standard deviations strips the units out and leaves a pure
number in `[-1, 1]`:

- `+1` — the two variables are an exact increasing linear function of each other
- `-1` — an exact decreasing linear function
- `0` — no linear relationship

Because a variable's correlation with itself is always `1`, the diagonal of the
matrix is all ones.

The input convention is the same as [`cov`](cov.md):

- a **2-D input** is **one variable per row**, observed across the columns. A
  `[V, N]` input returns a `[V, V]` correlation matrix.
- a **1-D input** is a single variable, and the result is the scalar `1`.

`corrcoef()` is exactly `cov()` with entry `(i, j)` divided by
`sd(i) * sd(j)`, then clamped into `[-1, 1]` to absorb rounding. The
`correction` divisor of `cov()` cancels in that ratio, so `corrcoef()` takes no
`correction` argument.

## Parameters

None.

## Returns

`Series`
    For a `[V, N]` input, a `[V, V]` symmetric correlation matrix in `float64`
    with a unit diagonal and every entry in `[-1, 1]`. For a 1-D input, the
    `float64` scalar `1`. Integer inputs are promoted to `float64` first.

## Gradient

Differentiable. It is a smooth composition of [`cov`](cov.md),
[`sqrt`](sqrt.md), reciprocal, and multiplication, so an objective built through
`corrcoef()` — a decorrelation penalty, a "match this target correlation" loss —
propagates back to the data. The gradient is finite only where every variance is
strictly positive (see the constant-row gotcha).

## Examples

Example 1. Two variables: the diagonal is `1`, the off-diagonal is their
correlation.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0.99437671],
  [0.99437671, 1]]
```

The two rows rise together almost perfectly, so their correlation is just under
`1`. Compare the covariance of the same data (`3.83`) — the same relationship,
but on an arbitrary scale.

Example 2. A variable that is an exact increasing linear function of another
correlates at exactly `1`.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {3.0, 5.0, 7.0, 9.0}});   // row1 = 2*row0 + 1
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [1, 1]]
```

The slope (`2`) and intercept (`1`) do not matter — correlation only sees the
shape of the relationship, not its scale or offset.

Example 3. An exact decreasing linear function correlates at `-1`.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {8.0, 6.0, 4.0, 2.0}});   // row1 = -2*row0 + 10
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, -1],
  [-1, 1]]
```

The sign of the off-diagonal is the sign of the slope. The clamp guarantees the
value is exactly `-1` and not `-1.0000000002`.

Example 4. There is no `correction` argument — it cancels in the ratio.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0.99437671],
  [0.99437671, 1]]
```

Identical to Example 1. Whether covariance and variance are divided by `N` or
`N - 1`, the same factor appears in numerator and denominator and drops out.

Example 5. A 1-D input is a single variable, which is perfectly correlated with
itself.

```cpp
std::cout << pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}).corrcoef();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
```

The result is a 0-D scalar, not a `1 x 1` matrix.

Example 6. The definition spelled out: `corrcoef` is `cov` divided by the outer
product of the standard deviations.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {2.0, 1.0, 4.0, 3.0},
                             {5.0, 6.0, 5.0, 6.0}});
auto c = x.cov();
auto sd = c.diagonal().sqrt();
std::cout << "manual   = " << c.div(sd.unsqueeze(1)).div(sd.unsqueeze(0));
std::cout << "corrcoef = " << x.corrcoef();
```

### Output

```text
manual   = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0.6, 0.4472136],
  [0.6, 1, -0.4472136],
  [0.4472136, -0.4472136, 1]]corrcoef = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0.6, 0.4472136],
  [0.6, 1, -0.4472136],
  [0.4472136, -0.4472136, 1]]
```

The two agree to full precision. The manual form is what `corrcoef()` does, plus
the `[-1, 1]` clamp.

Example 7. A constant row has zero variance, so its standard deviation is `0`
and its row and column come back `nan`.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {5.0, 5.0, 5.0, 5.0}});
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, nan],
  [nan, nan]]
```

Dividing by `sd = 0` produces `inf`, and `0 * inf` is `nan`. The `(0, 0)` entry
survives because variable `0` still has a well-defined variance.

Example 8. Gradient flows.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
x.tracks_slope_(true);
x.corrcoef().sum().reverse();
std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0.034587016, -0.017293508, -0.069174032, 0.051880524],
  [-0.017778373, 0.0064648628, 0.030708098, -0.019394588]]
```

The sensitivities are small because `sum(corrcoef(X))` is already near its
ceiling here — the two variables are almost perfectly correlated, so nudging the
data barely moves the result.

Example 9. A single `NaN` in the data poisons every entry whose row touches it.

```cpp
auto x = pluss::make_series({{1.0, 2.0, std::numeric_limits<double>::quiet_NaN(), 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
std::cout << x.corrcoef();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[nan, nan],
  [nan, 1]]
```

Row `0` contains the `NaN`, so entries `(0, 0)`, `(0, 1)`, and `(1, 0)` are
`nan`; `(1, 1)` depends only on the clean row `1` and stays `1`.

## Gotchas And Failure Modes

- **Rows are variables, columns are observations** — the same convention as
  [`cov`](cov.md). Transpose first if your observations are the rows.
- A **constant variable** (zero variance) gives a `nan` row and column; its
  standard deviation is `0` and the normalization divides by it. Drop such
  variables before calling.
- Off-diagonal entries are clamped into `[-1, 1]`. A value printed as exactly
  `1` or `-1` may have been `1 + 1e-12` before the clamp — do not read the clamp
  as an exact algebraic identity.
- Correlation only captures **linear** association. Two variables can be tightly
  related (for example `y = x^2` over a symmetric range) and still show a
  correlation near `0`.
- `NaN` propagates through the whole row and column it appears in. Filter or
  impute missing data first.
- The result is always `float64`.

## Summary

`corrcoef()` is the Pearson correlation matrix — [`cov`](cov.md) with the scale
divided out:

- 2-D input: **one variable per row**, `[V, N]` in, `[V, V]` matrix out with a
  unit diagonal and entries in `[-1, 1]`
- 1-D input: the scalar `1`
- no `correction` argument — it cancels in the ratio
- constant (zero-variance) variables produce `nan` rows and columns
- measures **linear** association only; differentiable where every variance is
  positive

## See Also

- [`Series::cov`](cov.md): the un-scaled version; `corrcoef()` is `cov()`
  divided by the outer product of the standard deviations.
- [`Series::standardize`](standardize.md): rescale each variable to zero mean
  and unit variance — do this to the rows and `cov()` becomes `corrcoef()`.
- [`Series::std`](std.md): the per-variable standard deviations that
  `corrcoef()` divides by.
- [`Series::center`](center.md): subtract the mean, the first step of both
  `cov()` and `corrcoef()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copysign.html">
    <span class="label">Previous</span>
    <span class="title">copysign</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
