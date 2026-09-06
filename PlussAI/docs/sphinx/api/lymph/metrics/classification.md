<!-- pluss-hand-authored -->

# `lymph::metrics::classification`

[Back to `metrics`](../metrics.md)

`metrics::classification` groups the helpers that score a **hard-label** pair —
a truth `Series` and a prediction `Series` you already hold. Nothing here trains
or runs a model.

- `confusion_table(truth, pred, ...)` → a [`ConfusionTable`](confusion_table.md)
- `summary(truth, pred, ...)` → a per-class [`ClassReport`](classification_summary.md)
- `report(truth, pred, {kinds}, ...)` → a [`MetricReport`](MetricReport.md) of
  the requested **hard-label** scalar metrics; `report(...).values` is the raw
  `Series`
- `score_report(scores, labels, {kinds})` → a [`MetricReport`](MetricReport.md)
  of the **score-based** metrics (`RocAuc`, `Gini`, `AveragePrecision`,
  `LogLoss`, `Brier`), straight from a probability `Series` — no model
- `threshold(scores, labels)` → a [`Threshold`](Threshold.md) operating-point
  accessor (also takes *scores*, not hard labels)

There is one spelling for each — `lymph::metrics::classification::confusion_table`
and so on. The old bare `lymph::confusion_table` / `lymph::classification_summary`
names are gone.

## Signature

```cpp
lymph::ConfusionTable lymph::metrics::classification::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const lymph::ConfusionOptions& options = {});

lymph::ClassReport lymph::metrics::classification::summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const lymph::ClassReportOptions& options = {});

lymph::metrics::MetricReport lymph::metrics::classification::report(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    std::vector<lymph::MetricKind> requested = {},
    const lymph::ClassReportOptions& options = {});

lymph::metrics::MetricReport lymph::metrics::classification::score_report(
    const pluss::Series& scores,
    const pluss::Series& labels,
    std::vector<lymph::MetricKind> requested = {});
```

`confusion_table` and `summary` each take a further overload with an explicit
`std::initializer_list<double> label_order` (plus a trailing `normalize` /
`zero_division`) in place of the options struct — see
[`confusion_table`](confusion_table.md) and
[`classification_summary`](classification_summary.md).

## Parameters

`truth_labels`
: Ground-truth class labels, as a `Series`.

`predicted_labels`
: Predicted class labels, same length as `truth_labels`.

`requested` — `std::vector<MetricKind>`
: Which scalar metrics to compute, in order. For `report`, empty → the
  classification default set (`Accuracy`, `BalancedAccuracy`, `Precision`,
  `Recall`, `F1`); `Mcc` and `CohenKappa` are also available (they come off the
  confusion matrix), but `Gini`, `AveragePrecision`, `Brier`, `RocAuc` and
  `LogLoss` are **not** — request those from `score_report` (or a model-backed
  [`reports`](../reports.md) call). For `score_report`, empty → `{RocAuc,
  AveragePrecision, LogLoss, Brier}`; any hard-label kind (`F1`, `Mcc`, …) is
  also accepted and scored at a 0.5 threshold / argmax.

`scores` / `labels` *(`score_report` only)*
: `scores` is a `[n]` `Series` of positive-class probabilities, or a `[n, k]`
  row-stochastic probability matrix for multiclass; `labels` is `[n]`, holding
  0/1 (binary) or class indices (multiclass). No model, no inference — the
  numbers come straight from the pair.

`options` — `lymph::ClassReportOptions`
: `label_order`, `zero_division` (the value precision / recall take on an empty
  denominator), `sample_weights`, and the balance-quality knobs. Applies to
  `summary` and `report`. See [`classification_summary`](classification_summary.md).

## Returns

- `confusion_table` → a [`ConfusionTable`](confusion_table.md).
- `summary` → a [`ClassReport`](classification_summary.md).
- `report` / `score_report` → a [`MetricReport`](MetricReport.md): `values`
  aligned to `requested`, plus a named 1-element `Series` per computed metric
  (`report.f1`, `report.mcc`, `report.roc_auc`, …).

## Errors

- `truth_labels` and `predicted_labels` (or `scores` and `labels`) of different
  lengths raise.
- `report` with a score-only `MetricKind` (`Gini`, `AveragePrecision`,
  `Brier`, `RocAuc`, `LogLoss`) raises — that path has no probabilities. Use
  `score_report` instead.
- `score_report` with a regression `MetricKind` (`Mse`, `R2`, …) raises.

## Practical Examples

<!-- these examples establish: the three label-pair helpers, report() -> MetricReport + .values, and options threading. -->

### Example 1

The three label-pair helpers over one truth / prediction pair.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ConfusionTable table = classification::confusion_table(truth, predicted);
  const lymph::ClassReport summary = classification::summary(truth, predicted);
  const pluss::Series values =
      classification::report(truth, predicted,
                             {lymph::MetricKind::Accuracy, lymph::MetricKind::F1})
          .values;

  std::cout << "confusion counts = " << table.counts << "\n";
  std::cout << "summary.f1       = " << summary.f1 << "\n";
  std::cout << "values (acc, f1) = " << values << "\n";

  return 0;
}
```

Output

```text
confusion counts = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 1],
  [1, 4]]
summary.f1       = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.8]
values (acc, f1) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.8]
```

### Example 2

`report({...})` bundles chosen metrics into a [`MetricReport`](MetricReport.md);
`report(...)` with no metric list uses the classification default set, and
`.values` is the raw `Series` behind it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::metrics::MetricReport report =
      classification::report(truth, predicted,
                             {lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
                              lymph::MetricKind::Recall, lymph::MetricKind::F1});
  std::cout << "report.accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "report.precision = " << report.precision.item<double>() << "\n";
  std::cout << "report.recall    = " << report.recall.item<double>() << "\n";
  std::cout << "report.f1        = " << report.f1.item<double>() << "\n";

  const pluss::Series defaults = classification::report(truth, predicted).values;
  std::cout << "default values (" << defaults.numel() << " metrics) = " << defaults << "\n";

  return 0;
}
```

Output

```text
report.accuracy  = 0.8
report.precision = 0.8
report.recall    = 0.8
report.f1        = 0.8
default values (5 metrics) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.8, 0.8, 0.8, 0.8, 0.8]
```

### Example 3

`ClassReportOptions` reaches `summary` — here `zero_division = 1.0` for a class
the model never predicts.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {10}, f64);

  const lymph::ClassReportOptions opts{.zero_division = 1.0};
  const lymph::ClassReport summary = classification::summary(truth, predicted, opts);

  std::cout << "labels    = " << summary.labels << "\n";
  std::cout << "precision = " << summary.precision << "  (class 1 uses zero_division)\n";
  std::cout << "recall    = " << summary.recall << "\n";

  return 0;
}
```

Output

```text
labels    = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
precision = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 1]  (class 1 uses zero_division)
recall    = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

### Example 4

`score_report(scores, labels, {…})` — the score-based metrics from a raw
probability / label pair, no model. Here `p = [.9 .8 .7 .6 .5]`,
`y = [1 0 1 1 0]`: ROC-AUC = 4/6 concordant pairs = 2/3, Gini = 2·AUC − 1 = 1/3,
AP = 29/36, LogLoss = 3.27544… / 5, Brier = mean (p − y)² = 1.15 / 5.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores =
      pluss::make_series({0.9, 0.8, 0.7, 0.6, 0.5}, {5}, f64);
  const pluss::Series labels =
      pluss::make_series({1, 0, 1, 1, 0}, {5}, f64);

  const lymph::metrics::MetricReport report = classification::score_report(
      scores, labels,
      {lymph::MetricKind::RocAuc, lymph::MetricKind::Gini,
       lymph::MetricKind::AveragePrecision, lymph::MetricKind::LogLoss,
       lymph::MetricKind::Brier});

  std::cout << "roc_auc           = " << report.roc_auc.item<double>() << "\n";
  std::cout << "gini              = " << report.gini.item<double>() << "\n";
  std::cout << "average_precision = " << report.average_precision.item<double>() << "\n";
  std::cout << "logloss           = " << report.logloss.item<double>() << "\n";
  std::cout << "brier             = " << report.brier.item<double>() << "\n";

  return 0;
}
```

Output

```text
roc_auc           = 0.666667
gini              = 0.333333
average_precision = 0.805556
logloss           = 0.655089
brier             = 0.23
```

## See Also

- [`confusion_table`](confusion_table.md) / [`classification_summary`](classification_summary.md)
  — the two table helpers, with their `label_order` overloads.
- [`Threshold`](Threshold.md) — `metrics::classification::threshold(scores, labels)`,
  the operating-point accessor.
- [`MetricReport`](MetricReport.md) — what `report` / `score_report` return.
- [`regression`](regression.md) — the regression twin.
- [`reports`](../reports.md) — the model-backed forms, which run inference and
  then call `score_report` for you.
