<!-- pluss-hand-authored -->

# `lymph::reports::threshold_curve` and `best_threshold`

[Back to `reports`](../reports.md)

The operating-point sweep, model-backed. `threshold_curve` returns
precision / recall / f1 at *every achievable* decision threshold;
`best_threshold` returns the single cutoff that maximizes one objective.
Both run the model to get scores for you — no materializing a score `Series`
by hand.

## Signature

```cpp
// models::Model + data -- four data shapes.
ThresholdCurve reports::threshold_curve(const models::Model&, const LabeledBatch&,          const InferOptions& = {});
ThresholdCurve reports::threshold_curve(const models::Model&, const DataFrame& x, const Series& y, const InferOptions& = {});
ThresholdCurve reports::threshold_curve(const models::Model&, const DataFrame& x, const Thread& y, const InferOptions& = {});
ThresholdCurve reports::threshold_curve(const models::Model&, const DataFrame& frame, const DataFrameBatchOptions&, const InferOptions& = {});

double reports::best_threshold(const models::Model&, /* same four shapes */ ..., MetricKind objective = MetricKind::F1, const InferOptions& = {});

// workflow::Workflow + Payload.
ThresholdCurve reports::threshold_curve(const workflow::Workflow&, const workflow::Payload&, const InferOptions& = {});
double         reports::best_threshold(const workflow::Workflow&, const workflow::Payload&, MetricKind objective = MetricKind::F1, const InferOptions& = {});

// Bound to a model, from model.reports():
ThresholdCurve reports::Model::threshold_curve(/* data */) const;
double         reports::Model::best_threshold(/* data */, MetricKind objective = MetricKind::F1) const;
```

## Parameters

`model` / `workflow`
: The trained artifact whose decision scores are swept.

`x` / `y` / `frame` / `data_options` / `batch` / `payload`
: The data, in any shape every other verb accepts — `(x, y)` with a `Series`
  or `Thread` label, `(frame, data_options)`, a `LabeledBatch` (`batch`), or
  a `Payload` for the `Workflow` overloads. `y` is the binary ground truth.

`objective` (`best_threshold` only) — `MetricKind`, default `MetricKind::F1`
: The metric to maximize. `Precision`, `Recall` / `Sensitivity`, and `F1`
  are honored; anything else falls back to `F1`.

`infer_options` (`lymph::options::Infer`)
: Passed to the score computation.

## Returns

`threshold_curve`
: A `ThresholdCurve` — `thresholds` (every achievable cutoff, descending),
  and `precision` / `recall` / `f1`, one value per threshold, aligned.

`best_threshold`
: A `double` — the threshold that maximizes `objective` over that sweep.
  Feed it back as `InferOptions::positive_threshold` at inference time.

## Errors

- Only meaningful for a **binary** classifier — the sweep is over one
  positive-class score. A multiclass or regression model is rejected by the
  underlying helper.
- The model / workflow must be trained.

## Practical Examples

### Example 1

`threshold_curve` — the full sweep. `slice` a few leading rows to see the
shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Thread(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::ThresholdCurve curve = lymph::reports::threshold_curve(model, x, y);
  std::cout << "threshold count = " << curve.thresholds.numel() << "\n";
  std::cout << "thresholds[0..4] = " << curve.thresholds.slice(0, 0, 4) << "\n";
  std::cout << "precision[0..4]  = " << curve.precision.slice(0, 0, 4) << "\n";
  std::cout << "recall[0..4]     = " << curve.recall.slice(0, 0, 4) << "\n";
  std::cout << "f1[0..4]         = " << curve.f1.slice(0, 0, 4) << "\n";

  return 0;
}
```

Output

```text
threshold count = 8
thresholds[0..4] = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 0.99940575, 0.00059423572]
precision[0..4]  = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 0.83333333]
recall[0..4]     = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.6, 0.8, 1, 1]
f1[0..4]         = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.75, 0.88888889, 1, 0.90909091]
```

### Example 2

`best_threshold` under three objectives — precision, recall, and F1 pick
different cutoffs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Thread(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  std::cout << "best_threshold(F1)        = "
            << lymph::reports::best_threshold(model, x, y, lymph::MetricKind::F1) << "\n";
  std::cout << "best_threshold(Recall)    = "
            << lymph::reports::best_threshold(model, x, y, lymph::MetricKind::Recall) << "\n";
  std::cout << "best_threshold(Precision) = "
            << lymph::reports::best_threshold(model, x, y, lymph::MetricKind::Precision) << "\n";

  return 0;
}
```

Output

```text
best_threshold(F1)        = 0.999406
best_threshold(Recall)    = 0.999406
best_threshold(Precision) = 1
```

### Example 3

The `Workflow` + `Payload` overload and the bound `model.reports()`
accessor.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {10}, f64))},
      {"income", Thread(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"}).label("default");

  std::cout << "workflow best_threshold(F1) = "
            << lymph::reports::best_threshold(trained, payload, lymph::MetricKind::F1) << "\n";

  lymph::models::Model model = lymph::models::logistic_regression();
  const DataFrame x({{"income", raw.features("income")}, {"balance", raw.features("balance")}});
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  model.learn_(x, y);
  std::cout << "accessor threshold count = " << model.reports().threshold_curve(x, y).thresholds.numel() << "\n";
  std::cout << "accessor best_threshold(F1) = "
            << model.reports().best_threshold(x, y, lymph::MetricKind::F1) << "\n";

  return 0;
}
```

Output

```text
workflow best_threshold(F1) = 0.999406
accessor threshold count = 8
accessor best_threshold(F1) = 0.999406
```

## See Also

- [`reports::Model`](Model.md) — the bound accessor.
- [`classification_summary`](classification_summary.md) — the fixed-threshold
  tables.
- [`metrics::classification::Threshold`](../metrics/Threshold.md) — the
  `Series`-level sweep, for when you already hold the score `Series`.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
