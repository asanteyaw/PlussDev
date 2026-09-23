# `pluss::ai::classic::cross_assess_sparse_binary` / `cross_assess_sparse_regression` / `cross_assess_sparse_multiclass`

Cross-validated assessment of the sparse linear learners — one function per
task, each fitting a fresh
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
[`SparseLinearRegressor`](SparseLinearRegressor.md) /
[`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) per fold and
scoring it on **both** that fold's train and test partitions — unlike
[`classic::cross_assess`](../metrics/cross_assess.md)'s single dense
`CrossAssessReport`, which only records test-side metrics, every
[`SparseFoldReport`](SparseCrossReport.md) here carries a `train_metrics`
alongside `metrics`. All three functions share one `assess_sparse_*_folds`
implementation internally (`assess_sparse_binary_folds` etc. are
undocumented aliases of the three functions below — call these, not those).

## Signature

```cpp
classic::StatusOr<classic::SparseCrossReport> classic::cross_assess_sparse_binary(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    const classic::LinearFamilyOptions& linear_options = {});

classic::StatusOr<classic::SparseCrossReport> classic::cross_assess_sparse_regression(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    const classic::LinearFamilyOptions& linear_options = {});

classic::StatusOr<classic::SparseCrossReport> classic::cross_assess_sparse_multiclass(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    const classic::LinearFamilyOptions& linear_options = {});
```

Each of the three takes only the `SparseTripletMatrix` form — there is no
`SparseCsrMatrix` overload here (the matrix is converted once internally via
[`sparse_triplet_to_csr`](make_sparse_triplet.md), then sliced per fold).

## Parameters

`x`
: The full matrix, sliced into train/test per fold. `rows` must match `y`'s
  length.

`y`
: One label/target per row, same conventions as the matching estimator's own
  `learn`: `cross_assess_sparse_binary` folds a third value toward `y`'s
  nearer extreme exactly like
  [`SparseLinearBinaryClassifier::learn`](SparseLinearBinaryClassifier.md#methods);
  `cross_assess_sparse_multiclass` fits one class per distinct value **within
  each fold's training rows** — see
  [Example 4](#example-4-a-fold-whose-training-rows-never-see-one-class)
  for what happens
  when a fold's test rows contain a class its own training rows never saw.

`split` (`SplitOptions`, default `{}`)
: The same five-way `plan` dispatch as
  [`classic::cross_assess`](../metrics/cross_assess.md#parameters) —
  `Holdout`, `CrossVal`, `StratifiedCrossVal`, `TSCrossVal`,
  `GroupCrossVal` — including the same `reliability_defaults` /
  `stratified` interaction (a classification task with `plan = Holdout`/
  `CrossVal` and `stratified` not already `true` gets stratification turned
  on automatically). **Unlike** dense `cross_assess`, `split.max_parallel`
  is not read at all — folds always run strictly sequentially, and the
  first fold that fails (fit or assess) aborts the whole call immediately;
  there is no partial `SparseCrossReport` for the folds that already
  succeeded.

`train_options` (`TrainOptions`, default `{}`)
: Forwarded to every fold's `learn` call unchanged — `max_iterations`/
  `tolerance` must satisfy the same `> 0` requirement as a direct
  [`learn`](SparseLinearBinaryClassifier.md#methods) call.

`assess_options` (`AssessOptions`, default `{}`)
: Forwarded to every fold's **two** `assess` calls (train and test).
  `assess_options.metrics` empty (the default) falls back to
  `{Accuracy}` for `cross_assess_sparse_binary`/`_multiclass`, or
  `{Mse, Rmse, R2}` for `cross_assess_sparse_regression` — the same
  per-task defaults each estimator's own `assess` uses.

`linear_options` (`LinearFamilyOptions`, default `{}`)
: One fixed value forwarded to **every** fold's `learn` call — there is no
  per-fold variation the way [`tune_sparse`](tune_sparse.md) sweeps several.
  Same ignored-field contract as the matching estimator's own `learn`
  (`objective`/`fit_intercept` ignored for binary/multiclass; see each
  class's own page).

## Returns

A `StatusOr<SparseCrossReport>` — see [`SparseCrossReport`](SparseCrossReport.md).
`folds.size()` matches the resolved split plan's fold count; `mean_metrics`/
`std_metrics` summarize the test side across folds, `mean_train_metrics`/
`std_train_metrics` the train side.

## Errors

- `y` undefined — `Status::invalid_argument`.
- `x`/`y` row-count mismatch — `Status::shape_mismatch`.
- Everything [`make_sparse_triplet`](make_sparse_triplet.md#errors) /
  the underlying split function
  ([`crossval_plan`](../split/crossval_plan.md#errors),
  [`holdout_split`](../split/holdout.md#errors), etc.) can already fail
  with.
- `assess_options.metrics` resolves to empty (only reachable by explicitly
  passing an empty vector after a non-empty one was already validated
  elsewhere) — `Status::invalid_argument`.
- Any fold's `learn`/`assess` failing — that `Status`, with a
  `"sparse cross assess fold i/n (train=..., test=...): "` prefix added to
  its message, propagated immediately (no other folds run after it).

## Practical Examples

Baseline 3-fold cross-assessment for each task, the train-vs-test metric
split unique to this report, and the multiclass fold-class-mismatch gotcha.

### Example 1

`cross_assess_sparse_binary` over 3 folds — `mean_metrics`/`std_metrics`
alongside the unique `mean_train_metrics`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      12, 2,
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {12}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::SparseCrossReport report = cls::cross_assess_sparse_binary(x, y, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  std::cout << "mean_metrics (test Accuracy)  = " << report.mean_metrics << "\n";
  std::cout << "mean_train_metrics (Accuracy) = " << report.mean_train_metrics << "\n";

  return 0;
}
```

Output

```text
fold count = 3
mean_metrics (test Accuracy)  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
mean_train_metrics (Accuracy) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

`cross_assess_sparse_regression` — default metrics fall back to
`{Mse, Rmse, R2}`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      10, 1, rows_v, std::vector<std::int64_t>(10, 0),
      {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0},
      {10}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;
  split.random.seed = 0;

  const cls::SparseCrossReport report = cls::cross_assess_sparse_regression(x, y, split).value();
  std::cout << "metrics.size() = " << report.metrics.size() << "\n";
  std::cout << "mean_metrics [Mse, Rmse, R2] = " << report.mean_metrics << "\n";

  return 0;
}
```

Output

```text
metrics.size() = 3
mean_metrics [Mse, Rmse, R2] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0005834719, 0.022730265, 0.99772317]
```

### Example 3

`cross_assess_sparse_multiclass` with `assess_options.metrics` set
explicitly to `{BalancedAccuracy}`, forwarded to every fold.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      9, 3,
      {0, 1, 2, 3, 4, 5, 6, 7, 8},
      {0, 0, 0, 1, 1, 1, 2, 2, 2},
      {1.0, 2.0, 3.0, 1.0, 2.0, 3.0, 1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0},
      {9}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::StratifiedCrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::AssessOptions balanced{};
  balanced.metrics = {cls::MetricKind::BalancedAccuracy};

  const cls::SparseCrossReport report =
      cls::cross_assess_sparse_multiclass(x, y, split, {}, balanced).value();
  std::cout << "mean_metrics (BalancedAccuracy) = " << report.mean_metrics << "\n";

  return 0;
}
```

Output

```text
mean_metrics (BalancedAccuracy) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 4: A Fold Whose Training Rows Never See One Class

`cross_assess_sparse_multiclass` fits `class_values_` from each fold's
**training** rows alone; if an unlucky split leaves a class entirely out of
one fold's training rows but present in its test rows,
[`assess`'s strict unseen-class rejection](SparseLinearMultiClassifier.md#methods)
fires inside that fold and aborts the whole call — a `Holdout` plan with a
tiny minority class deliberately forces that here.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // Class 2.0 has exactly one row (row 8) -- a plain (non-stratified)
  // holdout can easily place it entirely in the test partition.
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      9, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8}, std::vector<std::int64_t>(9, 0),
      {1.0, 1.1, 1.2, 2.0, 2.1, 2.2, 3.0, 3.1, 3.2}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0},
      {9}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions unstratified_holdout{};
  unstratified_holdout.plan = cls::CVPlanKind::Holdout;
  unstratified_holdout.reliability_defaults = false;  // keep it unstratified
  unstratified_holdout.holdout_fraction = 0.2;
  unstratified_holdout.shuffle = false;  // deterministic: last ~20% of rows become the test split

  const cls::StatusOr<cls::SparseCrossReport> report =
      cls::cross_assess_sparse_multiclass(x, y, unstratified_holdout);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "sparse cross assess fold 1/1 (train=7, test=2): SparseLinearMultiClassifier::assess labels must match classes observed at learn()."
```

## Changelog

- **0.1** — `cross_assess_sparse_binary`/`_regression`/`_multiclass`.

## See Also

- [`SparseCrossReport`](SparseCrossReport.md) — the returned type.
- [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
  [`SparseLinearRegressor`](SparseLinearRegressor.md) /
  [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — the
  estimators fit per fold.
- [`tune_sparse`](tune_sparse.md) / [`sweep_sparse`](sweep_sparse.md) — a
  budgeted/full search over `LinearFamilyOptions` built on this function.
- [`classic::cross_assess`](../metrics/cross_assess.md) — the dense
  counterpart (single `ModelSpec`, no separate train-side metrics).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
