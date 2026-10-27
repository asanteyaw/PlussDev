<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f8691f7ecd87 -->

# `lymph::selection::validate::Plan::scores`

[Back to `Plan`](../Plan.md)

`scores` runs the same cross-validation as [`run`](run.md) but returns just one
metric's value on each fold, as a `Series`. `score` is that `Series`' mean —
the single number you compare models on.

## Signature

```cpp
Series scores(const models::Model& model,
              const pluss::table::DataFrame& frame,
              const DataFrameBatchOptions& data_options,
              selection::Scorer objective,
              TrainOptions train_options = {}, AssessOptions assess_options = {}) const;

double score(/* identical arguments */) const;
```

Both take the same first-argument family as [`run`](run.md) — `models::Model`,
`ModelSpec`, or `workflow::Workflow` — over the same data shapes (`(frame,
data_options)`, `(x, y)`, `LabeledBatch`, `workflow::Payload`). The extra
argument is the [`Scorer`](../../Scorer.md): implicitly a `MetricKind`
(`plan.score(model, data, MetricKind::F1)`), or `selection::make_scorer("name",
fn)` for a callable scored on each fold's held-out `(truth, prediction)`.

The `models::Model` and `workflow::Workflow` overloads take a custom `Scorer`;
the `ModelSpec` overloads stay `MetricKind`-only (they run through the compiled
single-metric cross-validation path). A staged model (appended prep) needs the
`(frame, data_options)` shape for a custom `Scorer`, not the bare
`LabeledBatch` one — a nameless batch cannot route the prep stages.

## What It Does

`scores` calls `run` internally and pulls the named metric out of every fold, in
fold order, into a `Series` of length `repeats * folds`. `score` reduces that to
its arithmetic mean. Neither builds the full
[`reports::CrossVal`](../../../metrics/CrossValidationReport.md) surface — reach
for [`run`](run.md) when you want per-metric means, stdevs, and `board()`.

## Parameters

`model` / `spec` / `workflow`
: What to validate. Cloned per fold, exactly as in [`run`](run.md).

`frame` + `data_options` — or `x` + `y`, or `batch`, or `payload`
: The labelled data, in any shape [`run`](run.md) accepts.

`objective` — `lymph::selection::Scorer`
: The single value to collect per fold. Implicitly a `MetricKind` (`Accuracy`,
  `F1`, `R2`, `Mse`, …), or `selection::make_scorer("name", fn,
  greater_is_better)` for a custom callable. See [`Scorer`](../../Scorer.md).

`train_options` / `assess_options`
: Passed through to each per-fold fit and assessment.

## Returns

`scores` → a `pluss::Series` of per-fold values, in fold order.

`score` → a `double`, the mean of those values.

## Errors

Same as [`run`](run.md): a single-class test fold fails a classification metric;
a `time` plan needs oldest-first data through `(frame, data_options)`.

## Practical Examples

<!-- these examples establish: scores vs score, model comparison via score, and per-fold Series inspection. -->

### Example 1

`scores` for the raw per-fold accuracy; `score` for its mean.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan = lymph::selection::cross().folds(4).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const pluss::Series per_fold =
      plan.scores(model, frame, data_options, lymph::MetricKind::Accuracy);
  const double mean_accuracy =
      plan.score(model, frame, data_options, lymph::MetricKind::Accuracy);

  std::cout << "per-fold accuracy:\n" << per_fold << "\n";
  std::cout << "mean accuracy = " << mean_accuracy << "\n";

  return 0;
}
```

Output

```text
per-fold accuracy:
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.66666667, 1, 0.75]
mean accuracy = 0.854167
```

### Example 2

`score` is one number, so it slots straight into a model bake-off — one plan,
two estimators.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(5).repeat(2).shuffle(true).seed(20260901ULL).workers(1);

  const lymph::selection::CrossValOptions& o = plan.options();
  std::cout << "folds=" << o.folds << " repeats=" << o.repeats
            << " shuffle=" << std::boolalpha << o.shuffle
            << " seed=" << o.random_state
            << " deterministic=" << o.deterministic
            << " workers=" << o.max_parallel << "\n\n";

  std::cout << "logreg mean accuracy = "
            << plan.score(lymph::models::logistic_regression(), frame, data_options,
                          lymph::MetricKind::Accuracy)
            << "\n";
  std::cout << "tree   mean accuracy = "
            << plan.score(lymph::models::tree::decision_tree_classifier({.max_depth = 2}),
                          frame, data_options, lymph::MetricKind::Accuracy)
            << "\n";

  return 0;
}
```

Output

```text
folds=5 repeats=2 shuffle=true seed=20260901 deterministic=true workers=1

logreg mean accuracy = 0.925
tree   mean accuracy = 0.875
```

### Example 3

`scores` takes one `MetricKind` — here `F1` on a stratified plan — and the
returned `Series` reads out fold by fold via `host<double>()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series score = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8,
       3.0, 3.2, 3.4, 3.6, 3.8, 6.0, 6.4, 6.8, 7.2, 7.6}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Slate(score)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::stratified().folds(5).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const pluss::Series f1_per_fold =
      plan.scores(model, frame, data_options, lymph::MetricKind::F1);
  const double f1_mean =
      plan.score(model, frame, data_options, lymph::MetricKind::F1);

  const std::vector<double> folds = f1_per_fold.host<double>();
  std::cout << "fold count = " << folds.size() << "\n";
  for (std::size_t i = 0; i < folds.size(); ++i) {
    std::cout << "  fold " << i << " f1 = " << folds[i] << "\n";
  }
  std::cout << "mean f1 = " << f1_mean << "\n";

  return 0;
}
```

Output

```text
fold count = 5
  fold 0 f1 = 1
  fold 1 f1 = 1
  fold 2 f1 = 1
  fold 3 f1 = 1
  fold 4 f1 = 1
mean f1 = 1
```

### Example 4

A custom [`Scorer`](../../Scorer.md) as the objective — the **F2 score**
(F-beta with β = 2), which has no `MetricKind`. `scores` runs the callable on
each fold's held-out `(truth, prediction)`; `score` is its mean.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace selection = lymph::selection;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series score = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8,
       3.0, 3.2, 3.4, 3.6, 3.8, 6.0, 6.4, 6.8, 7.2, 7.6}, {20}, f64);
  const Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Slate(score)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  const auto f2 = [](const Series& truth, const Series& prediction) -> double {
    const Series t = truth.reshape({truth.numel()}).to(ScalarType::Float64);
    const Series p = prediction.reshape({prediction.numel()}).to(ScalarType::Float64);
    const double tp = t.eq(1.0).to(ScalarType::Float64).mul(p.eq(1.0).to(ScalarType::Float64)).sum().item<double>();
    const double fp = t.eq(0.0).to(ScalarType::Float64).mul(p.eq(1.0).to(ScalarType::Float64)).sum().item<double>();
    const double fn = t.eq(1.0).to(ScalarType::Float64).mul(p.eq(0.0).to(ScalarType::Float64)).sum().item<double>();
    const double precision = (tp + fp) > 0.0 ? tp / (tp + fp) : 0.0;
    const double recall = (tp + fn) > 0.0 ? tp / (tp + fn) : 0.0;
    const double denom = 4.0 * precision + recall;
    return denom > 0.0 ? 5.0 * precision * recall / denom : 0.0;
  };

  const selection::Plan plan = selection::stratified().folds(5).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const Series f2_per_fold =
      plan.scores(model, frame, data_options, selection::make_scorer("f2", f2));
  const double f2_mean =
      plan.score(model, frame, data_options, selection::make_scorer("f2", f2));

  std::cout << "f2 per fold = " << f2_per_fold << "\n";
  std::cout << "mean f2     = " << f2_mean << "\n";

  return 0;
}
```

Output

```text
f2 per fold = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 1, 1, 1]
mean f2     = 1
```

## See Also

- [`run`](run.md) — the full report, all metrics at once.
- [`Scorer`](../../Scorer.md) — the `objective` type; `make_scorer` for a custom
  metric.
- [`options::CrossVal`](../CrossValOptions.md) — the plan fields `scores` reads.
- [`cross`](../cross.md) / [`time`](../time.md) — the plan factories.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
  <a class="next-link" href="splits.html">
    <span class="label">Next</span>
    <span class="title">splits</span>
  </a>
</div>
<!-- apiref:nav:end -->
