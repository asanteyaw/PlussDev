<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55a0b1680a61 -->

# `lymph::models::Model::assess`

[Back to `lymph::models::Model`](../Model.md)

`assess` runs a trained model on a labelled frame and returns a `MetricReport`
comparing the predictions against the given labels.

## Signature

```cpp
lymph::MetricReport lymph::models::Model::assess(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    const lymph::options::Assess& options = {}) const;

lymph::MetricReport lymph::models::Model::assess(
    const pluss::table::DataFrame& x,
    const pluss::table::Thread& y,
    const lymph::options::Assess& options = {}) const;

lymph::MetricReport lymph::models::Model::assess(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Assess& options = {}) const;

lymph::MetricReport lymph::models::Model::assess(
    const lymph::LabeledBatch& batch,
    const lymph::options::Assess& options = {}) const;
```

## What It Does

`assess` is [`infer`](infer.md) followed by a scoring step. It replays the fitted
preparation over the feature columns, runs the estimator, compares the result
against `y`, and returns a `MetricReport`.

It **does not hold anything out**. The numbers describe the model's behaviour on
exactly the rows you pass. Handing it the training frame reports the training-set
fit; handing it a frame the model never saw reports generalisation (Example 3).
`assess` is the right tool for "how does this fitted model do on this specific
set of rows"; it is not model selection. Choosing between candidates under a
resampling policy belongs to the `selection` family and to
[`Builder`](../builder/Builder.md).

The input shapes mirror `learn` and `infer`: features plus a label `Series` or
`Thread`, one frame plus a `DFBatch` that names the label column, or a
pre-assembled `LabeledBatch`.

## The `MetricReport`

`MetricReport` has one field per metric — `accuracy`, `precision`, `recall`,
`f1`, `roc_auc`, `logloss` for classification; `rmse`, `mae`, `r2`, `mse`,
`quantile_loss` for regression; and a few more. Each is a one-element
`pluss::Series` read with `.item<double>()`.

A field is populated **only if its metric was requested**. With a default
`AssessOptions`, a small default set is filled and the rest stay undefined —
reading an undefined field throws. In practice: list the metrics you intend to
read in `options.metrics` (Examples 1 and 3).

## Input Shapes

The four `(features, label)` shapes mirror [`learn`](learn.md):

`const pluss::table::DataFrame& x, const pluss::Series& y`
: Feature columns in a frame, label in a numeric `Series`.

`const pluss::table::DataFrame& x, const pluss::table::Thread& y`
: Same, with the label as a `Thread`.

`const pluss::table::DataFrame& frame, const lymph::options::DFBatch& data_options`
: One frame carrying features and the label column; `data_options` names the
  label. Use the shape the model was trained with.

`const lymph::LabeledBatch& batch`
: A pre-assembled feature tensor and label vector.

## Parameters

### `options` — `lymph::options::Assess`

`std::vector<MetricKind> metrics` — default: a small built-in set
: The metrics to compute and populate. Set it to exactly what you will read.
  Values include `MetricKind::Accuracy`, `Precision`, `Recall`, `F1`, `RocAuc`,
  `LogLoss` (classification) and `Rmse`, `Mae`, `R2`, `Mse`, `QuantileLoss`
  (regression).

`double positive_threshold` — default `0.5`
: The probability cutoff for turning scores into hard labels, used by the metrics
  that need them (`Accuracy`, `F1`, `Precision`, `Recall`). Threshold-free
  metrics such as `RocAuc` ignore it (Example 2). Match it to the
  `positive_threshold` you will use at inference so the reported numbers reflect
  deployment.

`double regression_quantile` — default `0.5`
: The quantile `q` (with `0 < q < 1`) used when `MetricKind::QuantileLoss` is
  requested.

`pluss::Series sample_weights` — default undefined
: Per-row weights applied when aggregating the metrics, so an evaluation set with
  known row importance is scored accordingly.

`ObservationLayout layout` — default `ObservationLayout::Rows`
: Observation orientation, as elsewhere.

## Returns

A `MetricReport`. Only the requested metric fields are defined; the rest remain
`Series::undefined()`.

## Errors

- Reading a `MetricReport` field whose metric was not requested throws
  ("SeriesBase: undefined").
- The label length must match the frame's row count.
- `assess` on an untrained model, or on a `learn` template, raises
  ("Flow must be learned before infer").
- Requesting a regression metric for a classifier (or the reverse) produces an
  undefined field for that metric.

## Practical Examples

### Example 1

Classification metrics on a set with overlapping classes, so the numbers land
below `1.0`. Request the four you want; read each with `.item<double>()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Eight rows whose two classes overlap in the middle (rows 3 and 4 carry
  // swapped labels), so no straight boundary fits them all and the metrics
  // land below a perfect 1.0.
  const pluss::Series income = pluss::make_series({1.0, 1.6, 2.2, 2.8, 3.0, 3.6, 4.2, 4.8}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.4, 0.8, 1.2, 1.5, 1.6, 2.0, 2.4, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 0, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // assess runs inference, compares against the given labels, and returns a
  // MetricReport. Ask for the metrics you intend to read; unrequested
  // fields stay undefined. Each requested field is a one-element Series.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
                     lymph::MetricKind::Recall, lymph::MetricKind::F1};
  const lymph::MetricReport report = model.assess(x, y, options);
  std::cout << "accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "precision = " << report.precision.item<double>() << "\n";
  std::cout << "recall    = " << report.recall.item<double>() << "\n";
  std::cout << "f1        = " << report.f1.item<double>() << "\n";

  return 0;
}
```

Output

```text
accuracy  = 0.875
precision = 1
recall    = 0.75
f1        = 0.857143
```

### Example 2

`positive_threshold` moves the threshold-dependent metrics but not `roc_auc`.
The same model, same rows, assessed at `0.5` and at `0.8`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.6, 2.2, 2.8, 3.4, 4.0, 4.6, 5.2}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.4, 0.9, 1.3, 1.6, 2.0, 2.4, 2.8, 3.2}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 0, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // AssessOptions.metrics restricts the report to a chosen subset (the other
  // fields stay undefined). positive_threshold moves the hard-label boundary
  // that Accuracy and F1 depend on, exactly as it does for infer.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1, lymph::MetricKind::RocAuc};

  const lymph::MetricReport at_default = model.assess(x, y, options);

  options.positive_threshold = 0.8;
  const lymph::MetricReport at_strict = model.assess(x, y, options);

  std::cout << "threshold 0.5:  accuracy = " << at_default.accuracy.item<double>()
            << ", f1 = " << at_default.f1.item<double>()
            << ", roc_auc = " << at_default.roc_auc.item<double>() << "\n";
  std::cout << "threshold 0.8:  accuracy = " << at_strict.accuracy.item<double>()
            << ", f1 = " << at_strict.f1.item<double>()
            << ", roc_auc = " << at_strict.roc_auc.item<double>()
            << "   (roc_auc is threshold-free, so it does not move)\n";

  return 0;
}
```

Output

```text
threshold 0.5:  accuracy = 0.75, f1 = 0.75, roc_auc = 0.9375
threshold 0.8:  accuracy = 0.875, f1 = 0.857143, roc_auc = 0.9375   (roc_auc is threshold-free, so it does not move)
```

### Example 3

A regressor, and the "no hold-out" caveat. `assess` on the training rows and on
three unseen rows gives different error metrics — the second is the honest
estimate of generalisation.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // A regression target: roughly 2*hours + 1 with mild noise.
  const pluss::Series hours = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8}, f64);
  const pluss::Series score = pluss::make_series({3.2, 4.8, 7.1, 8.9, 11.2, 12.8, 15.3, 16.9}, {8}, f64);
  const DataFrame x_train({{"hours", Thread(hours)}});

  const lymph::models::Model model = lymph::models::linear_regression().learn(x_train, score);

  // For a regressor, assess reports error metrics instead of classification
  // ones. It does NOT hold anything out -- these numbers are the fit on the
  // rows it just learned from.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Rmse, lymph::MetricKind::Mae, lymph::MetricKind::R2};
  const lymph::MetricReport on_train = model.assess(x_train, score, options);

  // The same model measured against rows it never saw.
  const pluss::Series holdout_hours = pluss::make_series({2.5, 5.5, 9.0}, {3}, f64);
  const pluss::Series holdout_score = pluss::make_series({6.0, 11.8, 19.2}, {3}, f64);
  const DataFrame x_holdout({{"hours", Thread(holdout_hours)}});
  const lymph::MetricReport on_holdout = model.assess(x_holdout, holdout_score, options);

  std::cout << "on training rows:  rmse = " << on_train.rmse.item<double>()
            << ", mae = " << on_train.mae.item<double>()
            << ", r2 = " << on_train.r2.item<double>() << "\n";
  std::cout << "on held-out rows:  rmse = " << on_holdout.rmse.item<double>()
            << ", mae = " << on_holdout.mae.item<double>()
            << ", r2 = " << on_holdout.r2.item<double>() << "\n";

  return 0;
}
```

Output

```text
on training rows:  rmse = 0.185325, mae = 0.17381, r2 = 0.998363
on held-out rows:  rmse = 0.168261, mae = 0.146032, r2 = 0.99903
```

## See Also

- [infer](infer.md) — the prediction step `assess` scores.
- [`Builder`](../builder/Builder.md) — comparing several models under one
  policy.
- [`MetricReport`](../../metrics/CrossValidationReport.md) and the `metrics`
  family — the metric definitions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append.html">
    <span class="label">Previous</span>
    <span class="title">append</span>
  </a>
  <a class="next-link" href="feature_transformer.html">
    <span class="label">Next</span>
    <span class="title">feature_transformer</span>
  </a>
</div>
<!-- apiref:nav:end -->
