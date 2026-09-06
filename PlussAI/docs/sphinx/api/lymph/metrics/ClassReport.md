<!-- pluss-hand-authored -->

# `lymph::metrics::ClassReport`

[Back to `metrics`](../metrics.md)

`ClassReport` is the **per-class classification breakdown** —
precision / recall / f1 / support for every label, plus the accuracy and the
micro / macro / weighted average rows. It is what
[`metrics::classification::summary`](classification_summary.md) returns; the umbrella alias is
`lymph::reports::Class`. `ConfusionTable` — the raw count matrix — is documented
on this page too.

## Fields — `ClassReport`

All per-class `Series` are aligned to `labels`.

`Series labels`
: The discovered class labels, sorted (or in the requested `label_order`).

`Series support`
: True-instance count per class.

`Series precision` · `recall` · `f1` · `specificity` · `geometric_mean` ·
`balance_quality`
: One value per class.

`Series accuracy` · `mcc` · `cohen_kappa`
: Overall scalars — shape `[1]`, read with `.item<double>()`. `mcc` is the
  Matthews correlation coefficient and `cohen_kappa` the unweighted Cohen's
  kappa, both from the confusion matrix (`0` when their denominator vanishes —
  e.g. a single predicted class).

`Series micro_average` · `macro_average` · `weighted_average`
: Shape `[3]` — `[precision, recall, f1]`. `micro` pools all instances, `macro`
  is the unweighted mean over classes, `weighted` weights each class by its
  `support`.

`Series micro_balance` · `macro_balance` · `weighted_balance`
: Shape `[3]` — `[specificity, geometric_mean, balance_quality]`. The
  imbalance-aware counterpart of the average rows.

## Fields — `ConfusionTable`

`Series labels`
: Row / column label order.

`Series counts`
: The matrix, **row-major**: element `r*n + c` is "truth = `labels[r]`,
  prediction = `labels[c]`". Raw counts unless `normalize` was set, in which
  case the entries are rates.

`ConfusionNormalize normalize`
: `None`, `ByTruth` (rows sum to 1 — diagonal is recall), `ByPrediction`
  (columns sum to 1 — diagonal is precision), or `Global` (whole matrix sums to
  1).

## Producing Them

```cpp
lymph::ClassReport    lymph::metrics::classification::summary(
    truth, prediction, ClassReportOptions = {});
lymph::ConfusionTable lymph::metrics::classification::confusion_table(
    truth, prediction, ConfusionOptions = {});
```

Both also have a `{label_order}` initializer-list overload and model-backed
forms in [`reports`](../reports.md).

## Methods

None. Both `ClassReport` and `ConfusionTable` are plain value types — the
`Series` fields above are their whole surface.

## Reading a `ClassReport`

### Per-class rows

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 2, 1, 2, 2, 0, 2}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "support   = " << report.support << "\n";
  std::cout << "precision = " << report.precision << "\n";
  std::cout << "recall    = " << report.recall << "\n";
  std::cout << "f1        = " << report.f1 << "\n";

  return 0;
}
```

Output

```text
labels    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
support   = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 4, 4]
precision = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
recall    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
f1        = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
```

### The average rows

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 2, 1, 2, 2, 0, 2}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "accuracy         = " << report.accuracy.item<double>() << "\n";
  std::cout << "macro_average    = " << report.macro_average << "\n";
  std::cout << "weighted_average = " << report.weighted_average << "\n";
  std::cout << "micro_average    = " << report.micro_average << "\n";

  return 0;
}
```

Output

```text
accuracy         = 0.75
macro_average    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
weighted_average = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
micro_average    = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
```

### The balance rows

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "specificity    = " << report.specificity << "\n";
  std::cout << "geometric_mean = " << report.geometric_mean << "\n";
  std::cout << "balance_quality = " << report.balance_quality << "\n";
  std::cout << "macro_balance (spec, gmean, bq) = " << report.macro_balance << "\n";

  return 0;
}
```

Output

```text
specificity    = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.75, 0.875]
geometric_mean = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.81009259, 0.81009259]
balance_quality = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66445312, 0.64804688]
macro_balance (spec, gmean, bq) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.8125, 0.81009259, 0.65625]
```

## Changelog

- **0.1** — `ClassReport` (per-class rows + accuracy + average / balance rows)
  and `ConfusionTable` (`labels` / `counts` / `normalize`); umbrella alias
  `reports::Class`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
