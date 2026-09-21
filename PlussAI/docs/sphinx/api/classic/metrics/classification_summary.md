# `pluss::ai::classic::classification_summary`

Compute a full [`ClassReport`](ClassReport.md) from truth and predicted
class labels — every per-class and aggregate statistic
[`metrics`](../metrics.md) offers, in one call. Built directly on
[`confusion_table`](confusion_table.md); reach for `confusion_table` alone
when only the raw matrix matters, reach for this when you want the derived
statistics.

## Signature

```cpp
classic::StatusOr<classic::ClassReport> classic::classification_summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const classic::ClassReportOptions& options = {});

classic::StatusOr<classic::ClassReport> classic::classification_summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    /* a range of double */ label_order,
    double zero_division = 0.0);

classic::StatusOr<classic::ClassReport> classic::classification_summary(
    const classic::ClassifierBase& classifier,
    const classic::LabeledBatch& batch,
    const classic::InferOptions& infer_options = {},
    const classic::ClassReportOptions& options = {});
```

The second overload accepts anything convertible to a range of `double`
(including a plain `{...}` brace-initializer list) and forwards it as
`ClassReportOptions::label_order`, alongside `zero_division` spelled as a
direct parameter — the common case where you want to fix the class order
and the zero-division fallback without building a `ClassReportOptions` by
hand.

## Parameters

`truth_labels` / `predicted_labels`
: As [`confusion_table`](confusion_table.md)'s own — rank-any `Series` of
  class values, equal length, finite, non-empty.

`options` (`ClassReportOptions`, default `{}`)
: `label_order`, `zero_division`, `sample_weights`,
  `balance_quality_alpha`, `square_balance_quality` — see
  [`ClassReportOptions`](ClassReportOptions.md) for every field.

`classifier` / `batch` / `infer_options` (third overload)
: Runs `classifier.infer(batch.features, infer_options)` first, then scores
  `batch.y` against that prediction.

## Returns

A `StatusOr<ClassReport>` — see [`ClassReport`](ClassReport.md) for every
field.

## Errors

- `zero_division` not exactly `0.0` or `1.0` — `Status::invalid_argument`.
- `balance_quality_alpha` not finite or negative —
  `Status::invalid_argument`.
- Everything [`confusion_table`](confusion_table.md#errors) can fail with —
  the underlying matrix is computed first and any of its errors propagate
  directly.

## Practical Examples

The default report on an imbalanced 2-class problem (where `balance_quality`
and `specificity` diverge from the precision/recall/F1 picture), the
explicit `label_order` overload, the classifier-attached overload,
`sample_weights` changing which rows the confusion matrix (and therefore
every derived statistic) actually counts, and `balance_quality_alpha` /
`square_balance_quality` changing `balance_quality` on a case where recall
and specificity genuinely differ.

### Example 1

A 9-to-1 imbalanced problem: `recall` looks perfect for the majority class
but `specificity` (correctly identifying the *minority* class) reveals a
different picture.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 majority-class rows, all correct; 1 minority-class row, predicted wrong.
  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {10}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {10}, f64);

  const cls::ClassReport report = cls::classification_summary(truth, pred).value();
  std::cout << "accuracy   = " << report.accuracy << "\n";
  std::cout << "recall (per class)       = " << report.recall << "\n";
  std::cout << "specificity (per class)  = " << report.specificity << "\n";
  std::cout << "balance_quality (per class) = " << report.balance_quality << "\n";

  return 0;
}
```

Output

```text
accuracy   = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.9]
recall (per class)       = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
specificity (per class)  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
balance_quality (per class) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

### Example 2

`label_order` + `zero_division = 1.0`: a class absent from the data gets
`precision`/`recall` of `1.0` instead of the default `0.0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  const cls::ClassReport report =
      cls::classification_summary(truth, pred, {0.0, 1.0, 2.0}, 1.0).value();
  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "precision = " << report.precision << "\n";

  return 0;
}
```

Output

```text
labels    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
precision = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

### Example 3

The classifier-attached overload.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  const cls::ClassReport report = cls::classification_summary(*made.value(), batch).value();
  std::cout << "accuracy = " << report.accuracy << "\n";

  return 0;
}
```

Output

```text
accuracy = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 4

`sample_weights` feeds the underlying confusion matrix directly: zeroing
one row's weight removes it from every statistic exactly as if it were
never there. Row `1` (`truth=0`, misclassified as `1`) drags class `0`'s
recall down to `0.5` when every row counts equally; giving it weight `0`
recovers a perfect `1.0` for class `0`, since only row `0` (correct) still
counts.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0, 1.0, 1.0}, {4}, f64);

  const cls::ClassReport unweighted = cls::classification_summary(truth, pred).value();
  std::cout << "unweighted recall = " << unweighted.recall << "\n";

  cls::ClassReportOptions options{};
  options.sample_weights = pluss::make_series({1.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::ClassReport weighted = cls::classification_summary(truth, pred, options).value();
  std::cout << "weighted recall   = " << weighted.recall << "\n";

  return 0;
}
```

Output

```text
unweighted recall = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 1]
weighted recall   = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

### Example 5

`balance_quality_alpha` and `square_balance_quality` on a case where class
`1` has a real gap between recall (`0.667`) and specificity (`0.857`):
`alpha = 0` drops the gap penalty entirely (`balance_quality` reduces to
the geometric mean, squared or not); the default `alpha = 0.1` shaves a
little off for the gap; `square_balance_quality = false` uses the plain
geometric mean as the base term instead of its square.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Class 1: tp=2 (rows 7,8), fn=1 (row 9) -> recall = 2/3.
  // Class 1: tn=6 (rows 0-5), fp=1 (row 6) -> specificity = 6/7.
  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {10}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0}, {10}, f64);

  const cls::ClassReport default_report = cls::classification_summary(truth, pred).value();
  std::cout << "default (alpha=0.1, squared)   balance_quality = " << default_report.balance_quality << "\n";

  cls::ClassReportOptions no_penalty{};
  no_penalty.balance_quality_alpha = 0.0;
  const cls::ClassReport no_penalty_report = cls::classification_summary(truth, pred, no_penalty).value();
  std::cout << "alpha=0 (no gap penalty)       balance_quality = " << no_penalty_report.balance_quality << "\n";

  cls::ClassReportOptions unsquared{};
  unsquared.square_balance_quality = false;
  const cls::ClassReport unsquared_report = cls::classification_summary(truth, pred, unsquared).value();
  std::cout << "square_balance_quality=false   balance_quality = " << unsquared_report.balance_quality << "\n";

  return 0;
}
```

Output

```text
default (alpha=0.1, squared)   balance_quality = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.58231293, 0.56054422]
alpha=0 (no gap penalty)       balance_quality = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.57142857, 0.57142857]
square_balance_quality=false   balance_quality = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.77032759, 0.7415303]
```

## Changelog

- **0.1** — three overloads (`ClassReportOptions`, `label_order`+
  `zero_division`, classifier-attached).

## See Also

- [`ClassReport`](ClassReport.md) / [`ClassReportOptions`](ClassReportOptions.md) —
  the returned type and its options.
- [`confusion_table`](confusion_table.md) — the underlying matrix computation.
- [`classification_metrics`](classification_metrics.md) — a narrower,
  flat-`Series` view of a requested subset.
