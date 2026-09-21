# `pluss::ai::classic::ClassReport`

Every per-class and aggregate classification statistic this reference
computes, derived from one [`ConfusionTable`](ConfusionTable.md):
precision/recall/F1, specificity/geometric-mean/balance-quality, overall
accuracy, Matthews correlation coefficient, unweighted Cohen's kappa, and
micro/macro/weighted averages of both the precision-family and the
specificity-family metrics. Produced by
[`classification_summary`](classification_summary.md).

## Signature

```cpp
pluss::Series classic::ClassReport::labels;             // [k]
pluss::Series classic::ClassReport::support;             // [k]
pluss::Series classic::ClassReport::precision;           // [k]
pluss::Series classic::ClassReport::recall;              // [k]
pluss::Series classic::ClassReport::f1;                  // [k]
pluss::Series classic::ClassReport::specificity;         // [k]
pluss::Series classic::ClassReport::geometric_mean;      // [k]
pluss::Series classic::ClassReport::balance_quality;     // [k]
pluss::Series classic::ClassReport::accuracy;            // [1]
pluss::Series classic::ClassReport::mcc;                 // [1]
pluss::Series classic::ClassReport::cohen_kappa;         // [1]
pluss::Series classic::ClassReport::micro_average;       // [3] -> {precision, recall, f1}
pluss::Series classic::ClassReport::macro_average;       // [3] -> {precision, recall, f1}
pluss::Series classic::ClassReport::weighted_average;    // [3] -> {precision, recall, f1}
pluss::Series classic::ClassReport::micro_balance;       // [3] -> {specificity, geometric_mean, balance_quality}
pluss::Series classic::ClassReport::macro_balance;       // [3] -> {specificity, geometric_mean, balance_quality}
pluss::Series classic::ClassReport::weighted_balance;    // [3] -> {specificity, geometric_mean, balance_quality}
```

## Construction

Produced only by [`classification_summary`](classification_summary.md). A
plain value type — copy or store it freely.

## State

Every per-class field below is `Float64`, shape `[k]`, aligned to `labels`
(and to [`ConfusionTable::labels`](ConfusionTable.md#state) — this is the
same confusion matrix, seen through its derived statistics rather than raw
counts). `support[i]` is the row sum for class `i` (how many rows are truly
that class); `pred_support[i]` (not itself exposed, but used throughout) is
the column sum (how many rows were predicted that class).

`labels` — `pluss::Series`
: The class values, same order as `ConfusionTable::labels`.

`support` — `pluss::Series`
: How many rows truly belong to each class.

`precision` — `pluss::Series`
: `tp[i] / pred_support[i]` — of the rows predicted class `i`, the fraction
  that truly are. `zero_division` when `pred_support[i] == 0`.

`recall` — `pluss::Series`
: `tp[i] / support[i]` (sensitivity) — of the rows truly class `i`, the
  fraction predicted correctly. `zero_division` when `support[i] == 0`.

`f1` — `pluss::Series`
: The harmonic mean of `precision[i]` and `recall[i]`. `zero_division` when
  their sum is `0`.

`specificity` — `pluss::Series`
: `tn[i] / (tn[i] + fp[i])` — of the rows truly **not** class `i`, the
  fraction correctly predicted as something else. The true-negative
  counterpart to `recall`.

`geometric_mean` — `pluss::Series`
: `sqrt(recall[i] * specificity[i])` — a single number balancing sensitivity
  against specificity, insensitive to which one is being sacrificed for the
  other (unlike `f1`, which only trades precision against recall).

`balance_quality` — `pluss::Series`
: `geometric_mean[i]` (or its square, per
  [`square_balance_quality`](ClassReportOptions.md#square_balance_quality))
  scaled by `1 + balance_quality_alpha * (recall[i] - specificity[i])`,
  clamped to be non-negative. Rewards a class whose recall and specificity
  are both high *and* close to each other — see
  [`ClassReportOptions`](ClassReportOptions.md) for the two tuning fields.

`accuracy` — `pluss::Series`, shape `[1]`
: The overall fraction of rows predicted correctly — `sum(tp) / total`.

`mcc` — `pluss::Series`, shape `[1]`
: The multiclass Matthews correlation coefficient, computed directly from
  the confusion matrix's row/column sums (the same generalized formula
  regardless of class count — not a one-vs-rest average). `0` when the
  denominator's variance term is `0` (a degenerate confusion matrix, such as
  every row identical).

`cohen_kappa` — `pluss::Series`, shape `[1]`
: Unweighted Cohen's kappa, `(p_o - p_e) / (1 - p_e)`, where `p_o` is
  `accuracy` and `p_e` is the agreement expected from `support` and
  `pred_support` alone (chance agreement given the two marginal
  distributions). `0` when `p_e == 1` (a single-class confusion matrix).

`micro_average` — `pluss::Series`, shape `[3]`
: `{precision, recall, f1}` computed from *pooled* true/false positive/
  negative counts summed across every class first — equivalent to a
  single two-outcome (correct/incorrect) precision, so `micro_average`'s
  precision, recall, and F1 are always identical to each other and to
  `accuracy`.

`macro_average` — `pluss::Series`, shape `[3]`
: `{precision, recall, f1}` averaged **unweighted** across classes — every
  class counts equally regardless of its `support`.

`weighted_average` — `pluss::Series`, shape `[3]`
: `{precision, recall, f1}` averaged with each class weighted by
  `support[i] / total` — classes with more rows count more.

`micro_balance` / `macro_balance` / `weighted_balance` — `pluss::Series`, shape `[3]` each
: `{specificity, geometric_mean, balance_quality}`, aggregated the same
  three ways as `*_average` above.

## Methods

None — `ClassReport` is a plain data holder, read directly.

## Practical Example

A 3-class report showing per-class metrics next to the three averaging
strategies, and the two summary statistics computed straight from the
confusion matrix.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {9}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0}, {9}, f64);

  const cls::ClassReport report = cls::classification_summary(truth, pred).value();
  std::cout << "precision (per class) = " << report.precision << "\n";
  std::cout << "recall (per class)    = " << report.recall << "\n";
  std::cout << "macro_average    [p,r,f1] = " << report.macro_average << "\n";
  std::cout << "weighted_average [p,r,f1] = " << report.weighted_average << "\n";
  std::cout << "micro_average    [p,r,f1] = " << report.micro_average << "\n";
  std::cout << "accuracy   = " << report.accuracy << "\n";
  std::cout << "mcc        = " << report.mcc << "\n";
  std::cout << "cohen_kappa = " << report.cohen_kappa << "\n";

  return 0;
}
```

Output

```text
precision (per class) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.66666667, 0.75]
recall (per class)    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.66666667, 0.66666667, 1]
macro_average    [p,r,f1] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.80555556, 0.77777778, 0.77460317]
weighted_average [p,r,f1] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.80555556, 0.77777778, 0.77460317]
micro_average    [p,r,f1] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.77777778, 0.77777778, 0.77777778]
accuracy   = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.77777778]
mcc        = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.67936622]
cohen_kappa = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.66666667]
```

## Changelog

- **0.1** — `labels`, `support`, `precision`/`recall`/`f1`,
  `specificity`/`geometric_mean`/`balance_quality`, `accuracy`, `mcc`,
  `cohen_kappa`, micro/macro/weighted averages of both families.

## See Also

- [`classification_summary`](classification_summary.md) — the producing
  function.
- [`ClassReportOptions`](ClassReportOptions.md) — the options struct.
- [`ConfusionTable`](ConfusionTable.md) — the underlying matrix.
- [`classification_metrics`](classification_metrics.md) — a narrower,
  flat-`Series` view of a subset of these numbers.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
