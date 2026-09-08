<!-- pluss-hand-authored -->

# `lymph::reports::classification_summary` and `confusion_table`

[Back to `reports`](../reports.md)

The classification-tables trio, model-backed:
`classification_summary` → a [`ClassReport`](../metrics/ClassReport.md),
`confusion_table` → a `ConfusionTable` (labels + count matrix), and
`classification_metric_series` → the scalar metrics as one `Series`. Each
takes a trained [`models::Model`](../models/Model.md) or a
[`workflow::Workflow`](../workflow.md) plus data, runs the inference, and
scores — so you never thread `model.infer(x)` through by hand.

## Signature

```cpp
// models::Model + data -- three data shapes each.
ClassReport     reports::classification_summary(const models::Model&, const LabeledBatch&,          const InferOptions& = {}, const ClassReportOptions& = {});
ClassReport     reports::classification_summary(const models::Model&, const DataFrame& x, const Series& y, const InferOptions& = {}, const ClassReportOptions& = {});
ClassReport     reports::classification_summary(const models::Model&, const DataFrame& frame, const DataFrameBatchOptions&, const InferOptions& = {}, const ClassReportOptions& = {});

ConfusionTable  reports::confusion_table(const models::Model&, /* same three shapes */ ..., const ConfusionOptions& = {});
Series          reports::classification_metric_series(const models::Model&, /* same shapes + Thread-y */ ..., const AssessOptions& = {});

// workflow::Workflow + Payload.
ClassReport     reports::classification_summary(const workflow::Workflow&, const workflow::Payload&, const InferOptions& = {}, const ClassReportOptions& = {});
ConfusionTable  reports::confusion_table(const workflow::Workflow&, const workflow::Payload&, const InferOptions& = {}, const ConfusionOptions& = {});
Series          reports::classification_metric_series(const workflow::Workflow&, const workflow::Payload&, const AssessOptions& = {});

// The same calls, bound to a model, from model.reports():
ClassReport    reports::Model::classification_summary(/* data */) const;
ConfusionTable reports::Model::confusion_table(/* data */) const;
Series         reports::Model::metric_series(/* data */) const;
```

## Parameters

`model` / `workflow`
: The trained artifact. A `Workflow` runs its `Policy` + prep + model before
  scoring — the same evidence for a whole deployable artifact.

`x` / `y` / `frame` / `data_options` / `batch` / `payload`
: The data, in any of the shapes every other verb accepts — `(x, y)` (a
  `Series` label), `(frame, data_options)` (a `DataFrameBatchOptions` naming
  the columns), a `LabeledBatch` (`batch`), or a `Payload` for the
  `Workflow` overloads. The label side is the ground truth to score
  against.

`infer_options` (`lymph::options::Infer`)
: Passed to the inference step — most usefully `positive_threshold`.

`options` — `ClassReportOptions` / `ConfusionOptions` / `AssessOptions`
: The scoring configuration for the matching
  [`metrics`](../metrics.md) helper: `zero_division`, `label_order`,
  `normalize`, `metrics` list, and so on.

## Returns

`classification_summary`
: A [`ClassReport`](../metrics/ClassReport.md) — per-class `precision` /
  `recall` / `f1` / `support`, plus `[1]`-shaped `accuracy` / `mcc` /
  `cohen_kappa` and the `micro` / `macro` / `weighted` averages.

`confusion_table`
: A `ConfusionTable` — `labels` (the class order) and `counts` (the
  `n_classes × n_classes` matrix, `normalize` controls raw vs. rate).

`classification_metric_series`
: A `Series` of the scalar metrics named in `AssessOptions::metrics`
  (task default when unset), in that order.

## Errors

- The model / workflow must be trained; an untrained one raises the same
  "must be learned" error `infer` does.
- A regression model has no classification report — the underlying
  `metrics::classification` helper rejects it.
- Everything `infer` can raise for the data shape (missing feature column,
  unseen category) applies.

## Practical Examples

### Example 1

`classification_summary` matches the by-hand
`metrics::classification::summary(y, model.infer(x))` exactly — the facade
just folds in the `infer`.

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

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, data_options);

  const lymph::ClassReport via_reports =
      lymph::reports::classification_summary(model, frame, data_options);

  const DataFrame x({{"income", frame.features("income")}, {"balance", frame.features("balance")}});
  const Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const lymph::ClassReport by_hand =
      lymph::metrics::classification::summary(y, model.infer(x));

  std::cout << "reports accuracy = " << via_reports.accuracy.item<double>() << "\n";
  std::cout << "by-hand accuracy = " << by_hand.accuracy.item<double>() << "\n";
  std::cout << "reports f1        =\n" << via_reports.f1 << "\n";
  std::cout << "reports mcc      = " << via_reports.mcc.item<double>() << "\n";

  return 0;
}
```

Output

```text
reports accuracy = 1
by-hand accuracy = 1
reports f1        =
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
reports mcc      = 1
```

### Example 2

`confusion_table`, and the same call off the bound `model.reports()`
accessor.

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
      {"income", Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::ConfusionTable table = lymph::reports::confusion_table(model, x, y);
  std::cout << "labels =\n" << table.labels << "\n";
  std::cout << "counts =\n" << table.counts << "\n";

  const lymph::ConfusionTable via_accessor = model.reports().confusion_table(x, y);
  std::cout << "accessor counts =\n" << via_accessor.counts << "\n";

  return 0;
}
```

Output

```text
labels =
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
counts =
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 0],
  [0, 4]]
accessor counts =
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 0],
  [0, 4]]
```

### Example 3

The `Workflow` + `Payload` overload — evidence for the deployable artifact,
`Policy` and all. `classification_metric_series` returns the scalar metrics
as one `Series`.

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
      {"id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
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

  const lymph::ClassReport report = lymph::reports::classification_summary(trained, payload);
  std::cout << "workflow accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "workflow f1       =\n" << report.f1 << "\n";

  const pluss::Series metric_series =
      lymph::reports::classification_metric_series(trained, payload);
  std::cout << "metric_series =\n" << metric_series << "\n";

  return 0;
}
```

Output

```text
workflow accuracy = 1
workflow f1       =
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
metric_series =
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

## See Also

- [`reports::Model`](Model.md) — the bound accessor these calls also live on.
- [`threshold_curve`](threshold_curve.md) — the operating-point sweep.
- [`metrics::classification`](../metrics/classification_summary.md) — the
  compute layer, for when you already hold the prediction `Series`.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
