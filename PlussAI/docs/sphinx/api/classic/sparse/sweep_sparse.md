# `pluss::ai::classic::sweep_sparse_binary` / `sweep_sparse_regression` / `sweep_sparse_multiclass`

Rank a list of [`LinearFamilyOptions`](../families/LinearFamilyOptions.md)
candidates for a sparse task. **Presently identical to
[`tune_sparse_binary`/`_regression`/`_multiclass`](tune_sparse.md)** — each
function here calls its `tune_sparse_*` counterpart with the exact same
arguments and returns its result unchanged (confirmed directly in
[Example 1](#example-1)). The two names exist because dense
[`classic`](../tune.md) draws a real distinction between
[`sweep`](../tune/sweep.md) (always exhaustive) and
[`tune`](../tune/tune.md) (budgeted, strategy-selectable); the sparse family
has no budgeted/strategy-selectable search at all — every candidate is
always evaluated — so that distinction currently has nothing to attach to
here. Prefer `sweep_sparse_*` when the exhaustive-evaluation intent matters
to a reader, `tune_sparse_*` otherwise; both cost and do exactly the same
thing today.

## Signature

```cpp
classic::StatusOr<classic::SparseTuneReport> classic::sweep_sparse_binary(
    /* a range of LinearFamilyOptions -- pass a std::vector<LinearFamilyOptions> or braced list */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);

classic::StatusOr<classic::SparseTuneReport> classic::sweep_sparse_regression(
    /* a range of LinearFamilyOptions */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Mse,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);

classic::StatusOr<classic::SparseTuneReport> classic::sweep_sparse_multiclass(
    /* a range of LinearFamilyOptions */ candidates,
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::SplitOptions& split = {},
    const classic::TrainOptions& train_options = {},
    const classic::AssessOptions& assess_options = {},
    classic::MetricKind objective_metric = classic::MetricKind::Accuracy,
    classic::SparseObjectivePolicy objective_policy = classic::SparseObjectivePolicy::Auto);
```

Unlike [`tune_sparse_binary`/`_regression`/`_multiclass`](tune_sparse.md),
there is no braced-list convenience overload here — only the range-based
form. Pass a `std::vector<LinearFamilyOptions>` (a `{...}` brace list
converts to one implicitly at the call site, same as any `std::vector`
argument).

## What It Does

Cross-validates every candidate in `candidates` against `x`/`y` and ranks
them by `objective_metric`, exhaustively -- every candidate is always
evaluated, with no early stopping or budgeted search. The result identifies
which set of `LinearFamilyOptions` performs best on this data, without
requiring a separate manual loop over candidates.

## Parameters

Identical, argument for argument, to
[`tune_sparse_binary`/`_regression`/`_multiclass`](tune_sparse.md#parameters) —
see that page for the full contract on each; every parameter here is
forwarded to the matching `tune_sparse_*` function unchanged:

`candidates` / `x` / `y` / `split` / `train_options` / `assess_options` /
`objective_metric` / `objective_policy`
: See [`tune_sparse`'s own parameter docs](tune_sparse.md#parameters) —
  nothing here behaves differently.

## Returns

A `StatusOr<SparseTuneReport>` — see [`SparseTuneReport`](SparseTuneReport.md).
Bit-for-bit identical to calling the matching `tune_sparse_*` function with
the same arguments.

## Errors

Identical to [`tune_sparse_binary`/`_regression`/`_multiclass`](tune_sparse.md#errors).

## Practical Examples

The identity with `tune_sparse_*` proven directly for one task, then one
ordinary baseline sweep for each of the other two.

### Example 1

`sweep_sparse_binary` and `tune_sparse_binary`, same arguments, same
`SparseTuneReport` in every field that matters.

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

  std::vector<cls::LinearFamilyOptions> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.regularization = cls::RegularizationKind::L2;
    opts.regularization_strength = strength;
    candidates.push_back(opts);
  }

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::SparseTuneReport via_sweep = cls::sweep_sparse_binary(candidates, x, y, split).value();
  const cls::SparseTuneReport via_tune = cls::tune_sparse_binary(candidates, x, y, split).value();
  std::cout << "best_index matches = " << std::boolalpha << (via_sweep.best_index == via_tune.best_index) << "\n";
  std::cout << "trial count matches = " << (via_sweep.trials.size() == via_tune.trials.size()) << "\n";

  return 0;
}
```

Output

```text
best_index matches = true
trial count matches = true
```

### Example 2

`sweep_sparse_regression` over two `regularization` settings.

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

  std::vector<cls::LinearFamilyOptions> candidates{};
  candidates.push_back(cls::LinearFamilyOptions{});
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 0.5;
  candidates.push_back(l2);

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;
  split.random.seed = 0;

  const cls::SparseTuneReport report =
      cls::sweep_sparse_regression(candidates, x, y, split, {}, {}, cls::MetricKind::Mse).value();
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
best_index = 0
```

### Example 3

`sweep_sparse_multiclass` over two `regularization` settings, ranked by
`BalancedAccuracy`.

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

  std::vector<cls::LinearFamilyOptions> candidates{};
  candidates.push_back(cls::LinearFamilyOptions{});
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  candidates.push_back(l2);

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::StratifiedCrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::AssessOptions balanced{};
  balanced.metrics = {cls::MetricKind::BalancedAccuracy};

  const cls::SparseTuneReport report =
      cls::sweep_sparse_multiclass(candidates, x, y, split, {}, balanced, cls::MetricKind::BalancedAccuracy).value();
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
```

Output

```text
best_index = 0
```

## Changelog

- **0.1** — `sweep_sparse_binary`/`_regression`/`_multiclass`, each a direct
  forward to the matching `tune_sparse_*` function.

## See Also

- [`tune_sparse`](tune_sparse.md) — the function this presently forwards to
  unchanged.
- [`SparseTuneReport`](SparseTuneReport.md) — the returned type.
- [`sweep`](../tune/sweep.md) — the dense counterpart, genuinely distinct
  from dense [`tune`](../tune/tune.md).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
