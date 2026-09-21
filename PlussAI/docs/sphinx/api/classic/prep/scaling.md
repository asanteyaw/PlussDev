# `pluss::ai::classic::prep::scaling`

`scaling` owns rescaling: changing the numeric magnitude of each feature
column without changing its distribution's shape (aside from
[`RobustScaler`](scaling/RobustScaler.md)'s robustness to outliers) or which
row is largest within a column. Every scaler here fits one number pair per
feature during `learn` — a center and a spread, or a min and a max — and
applies the same affine formula to every value during `map`.

`scaling` does not touch categorical columns (see [`encoding`](encoding.md)),
does not fill in missing values (see [`imputation`](imputation.md)), and does
not change a distribution's shape beyond a linear rescale (see
[`transform`](transform.md) for power/quantile reshaping).

## The Surface

| Type | Centers on | Spreads by | Reach for it when |
| --- | --- | --- | --- |
| [`StandardScaler`](scaling/StandardScaler.md) | the mean | the standard deviation | the default choice — most models (linear, SVM, k-means, PCA) assume roughly zero-centered, unit-scale input |
| [`MinMaxScaler`](scaling/MinMaxScaler.md) | the minimum | the min-max range | you need output confined to a known, fixed interval (e.g. `[0, 1]` for a sigmoid input) |
| [`RobustScaler`](scaling/RobustScaler.md) | the median | the interquartile range | the column has outliers that would otherwise dominate a mean/stddev fit |
| [`MaxAbsScaler`](scaling/MaxAbsScaler.md) | nothing (no centering) | the maximum absolute value | the column is already meaningfully centered at zero (e.g. it is signed) and sparsity must be preserved — zeros stay zero |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::prep::StandardScaler scaler;
scaler.learn(train_batch);
const pluss::Series train_scaled = scaler.map(train_batch).value();
const pluss::Series test_scaled = scaler.map(test_batch).value();  // same fitted center/scale
```

Every scaler shares this shape — see [`scaling/Introduction`](scaling/Introduction.md)
for the full contract (fitted state, `NaN` handling, degenerate-feature
fallback, persistence) common to all four.

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`compose`](../compose.md) — `Flow::add_standard_scaler` and its three
  siblings, routing a scaler into a larger pipeline.
- [`estimators`](../estimators.md) — `TransformerBase`, the interface every
  scaler implements.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

scaling/Introduction
scaling/StandardScaler
scaling/MinMaxScaler
scaling/RobustScaler
scaling/MaxAbsScaler
```
<!-- apiref:toctree:end -->
