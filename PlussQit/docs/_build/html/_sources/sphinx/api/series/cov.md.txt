<!-- pluss-hand-authored -->

# Series::cov

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cov() const
Series Series::cov(std::int64_t correction) const
```

## What It Does

Builds the **covariance matrix** of a set of variables.

Covariance measures how two quantities move together. If a variable is centered
(its mean subtracted), the average product of two centered variables is their
covariance: large and positive when they rise and fall in step, large and
negative when one rises as the other falls, near zero when they are unrelated.
`cov()` computes that number for every pair of variables at once and lays the
results out in a square matrix.

The layout convention is the part to get right:

- a **2-D input** is read as **one variable per row**, observed across the
  columns. A `[V, N]` input is `V` variables with `N` observations each, and
  `cov()` returns a `[V, V]` matrix whose `(i, j)` entry is the covariance of
  variable `i` with variable `j`.
- a **1-D input** is a single variable, and `cov()` returns its scalar
  variance.

The diagonal of the matrix is each variable's own variance (a variable's
covariance with itself). The matrix is always symmetric: entry `(i, j)` equals
entry `(j, i)`.

Internally each row is centered with [`center`](center.md) and the matrix is
formed as `Xc @ Xc^T` divided by `N - correction`, all on the differentiable
path, so `cov()` can sit inside an objective you optimize.

## Parameters

**correction** (`std::int64_t`, default `1`): the divisor is
`num_observations - correction`.

- `correction = 1` (the default) is the **sample** covariance — the unbiased
  estimator, dividing by `N - 1`.
- `correction = 0` is the **population** covariance, dividing by `N`.

Any integer is accepted; `N - correction <= 0` yields non-finite entries.

## Returns

`Series`
    For a `[V, N]` input, a `[V, V]` symmetric covariance matrix in `float64`.
    For a 1-D input, a `float64` scalar variance. Integer inputs are promoted to
    `float64` first.

## Gradient

Differentiable. The centering and the `Xc @ Xc^T` product are both on the taped
path, so an objective built through `cov()` (a risk penalty, a decorrelation
loss, a whitening term) propagates sensitivities back to the data.

## Examples

Example 1. Two variables, four observations each. Rows are the variables.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
std::cout << x.cov();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.6666667, 3.8333333],
  [3.8333333, 8.9166667]]
```

The `(0, 1)` and `(1, 0)` entries are equal — covariance is symmetric. Both are
positive here because the two rows increase together.

Example 2. A 1-D input is one variable, so `cov()` is its scalar variance.

```cpp
std::cout << pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}).cov();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.5714286
```

The mean is `5`; the squared deviations sum to `32`; `32 / (8 - 1) = 4.571...`.

Example 3. `correction` sets the divisor. The default divides by `N - 1`;
`correction = 0` divides by `N`.

```cpp
auto x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});
std::cout << "N-1: " << x.cov();
std::cout << "N  : " << x.cov(0);
```

### Output

```text
N-1: pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.5714286N  : pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

`32 / 8 = 4` exactly. The `N - 1` form is larger — it inflates the estimate to
correct for the bias of using the sample mean.

Example 4. The diagonal is exactly each row's variance.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
std::cout << "cov diag = " << x.cov().diagonal();
std::cout << "row0 var = " << x.slice(0, 0, 1).reshape({4}).var();
std::cout << "row1 var = " << x.slice(0, 1, 2).reshape({4}).var();
```

### Output

```text
cov diag = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.6666667, 8.9166667]row0 var = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.6666667]row1 var = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[8.9166667]
```

[`var`](var.md) also defaults to the `N - 1` divisor, so the numbers line up.

Example 5. The matrix is symmetric — `C` minus its transpose is all zero.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {2.0, 1.0, 4.0, 3.0},
                             {5.0, 6.0, 5.0, 6.0}});
auto c = x.cov();
std::cout << c;
std::cout << "max|C - C^T| = " << c.sub(c.transpose(0, 1)).abs().max();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1.6666667, 1, 0.33333333],
  [1, 1.6666667, -0.33333333],
  [0.33333333, -0.33333333, 0.33333333]]max|C - C^T| = pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

Three variables give a `3 x 3` matrix. The third row is nearly constant, so its
variance (`0.333`) is small.

Example 6. Gradient flows: `cov()` is on the differentiable path.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {1.0, 3.0, 5.0, 8.0}});
x.tracks_slope_(true);
x.cov().sum().reverse();
std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-3.1666667, -1.1666667, 0.83333333, 3.5],
  [-3.1666667, -1.1666667, 0.83333333, 3.5]]
```

The sensitivity of `sum(cov(X))` to observation `j` is proportional to the sum,
across variables, of that observation's centered value — which is why both rows
receive the same gradient here.

Example 7. Two variables that move in opposite directions have a negative
off-diagonal.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                             {4.0, 3.0, 2.0, 1.0}});
std::cout << x.cov();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.6666667, -1.6666667],
  [-1.6666667, 1.6666667]]
```

The rows are exact mirror images, so the off-diagonal is `-1` times the shared
variance. The diagonal is unchanged — variance never sees the sign.

Example 8. Three or more dimensions are rejected.

```cpp
try {
  std::cout << pluss::series::zeros({2, 2, 2}).cov();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
cov: expected a 1D or 2D input
```

## Gotchas And Failure Modes

- **Rows are variables, columns are observations.** If your data is stored the
  other way (one row per observation), transpose it first:
  `x.transpose(0, 1).cov()`. Getting this backwards silently computes the
  covariance of the wrong thing.
- The default divisor is `N - 1`, not `N`. Pass `cov(0)` for the population
  form. This matches [`var`](var.md) and [`std`](std.md).
- With a single observation (`N = 1`) the default divisor is `0` and every
  entry is non-finite.
- `NaN` in any observation propagates through the centering and the product, so
  one missing value can poison an entire row and column of the result. Filter
  or impute first.
- The result is always `float64`. An integer input is promoted; it is not kept
  integral.
- `cov()` does not weight observations. For a weighted covariance, scale the
  centered data yourself.

## Summary

`cov()` is the covariance matrix of a set of variables:

- 2-D input: **one variable per row**, `[V, N]` in, `[V, V]` symmetric matrix
  out
- 1-D input: a single variable, scalar variance out
- diagonal entries are each variable's variance
- divisor is `N - correction`; `correction = 1` (default) is the unbiased
  sample form, `correction = 0` the population form
- differentiable; `float64` result

## See Also

- [`Series::corrcoef`](corrcoef.md): `cov()` rescaled so the diagonal is `1`
  and every entry lies in `[-1, 1]` — covariance with the units divided out.
- [`Series::var`](var.md) / [`Series::std`](std.md): the single-variable
  spread; the diagonal of `cov()`.
- [`Series::center`](center.md): subtract the mean — the first step `cov()`
  performs internally.
- [`Series::outer`](outer.md): the un-centered, un-scaled rank-1 product that
  `cov()` generalizes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cosh.html">
    <span class="label">Previous</span>
    <span class="title">cosh</span>
  </a>
  <a class="next-link" href="cross.html">
    <span class="label">Next</span>
    <span class="title">cross</span>
  </a>
</div>
