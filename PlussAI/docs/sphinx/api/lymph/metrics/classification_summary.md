<!-- pluss-hand-authored -->

# `lymph::metrics::classification::summary`

[Back to `metrics`](../metrics.md)

`metrics::classification::summary(truth, prediction)` computes a
[`ClassReport`](ClassReport.md) from two label `Series` — per-class
precision / recall / f1 / support plus the accuracy and the average rows. There is one
spelling — the old bare `lymph::classification_summary` name has been removed.

## Signature

```cpp
lymph::ClassReport lymph::metrics::classification::summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const lymph::ClassReportOptions& options = {});

lymph::ClassReport lymph::metrics::classification::summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    std::initializer_list<double> label_order,
    double zero_division = 0.0);
```

## What It Does

Discovers the class labels, then computes precision, recall, f1, specificity,
geometric mean and balance quality per class, the overall accuracy, and the
micro / macro / weighted average rows (see [`ClassReport`](ClassReport.md) for
the field shapes). `zero_division` is the value precision or recall takes when
its denominator is empty — e.g. a class the model never predicts.

## Parameters

`truth_labels`
: Ground-truth class labels.

`predicted_labels`
: Predicted class labels, same length.

`options` — `lymph::ClassReportOptions`
: `label_order` (a `Series` fixing the class order), `zero_division` (default
  `0.0`), `sample_weights`, and `balance_quality_alpha` /
  `square_balance_quality` (the balance-quality shape knobs).

`label_order` / `zero_division` *(initializer-list overload)*
: The class order as a `{…}` list, and the empty-denominator fallback, without
  building a `ClassReportOptions`.

## Returns

A `lymph::ClassReport` (umbrella alias `reports::Class`).

## Errors

- Length mismatch between the two `Series` raises.

## Practical Examples

<!-- these examples establish: a binary summary, the zero_division fallback, and a multiclass weighted average. -->

### Example 1

A binary summary — per-class rows plus accuracy and the macro average.

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
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "support   = " << report.support << "\n";
  std::cout << "precision = " << report.precision << "\n";
  std::cout << "recall    = " << report.recall << "\n";
  std::cout << "f1        = " << report.f1 << "\n";
  std::cout << "accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "macro_average (p,r,f1) = " << report.macro_average << "\n";

  return 0;
}
```

Output

```text
labels    = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
support   = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 5]
precision = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.8]
recall    = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.8]
f1        = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.8, 0.8]
accuracy  = 0.8
macro_average (p,r,f1) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.8, 0.8, 0.8]
```

### Example 2

`zero_division` decides the value of an undefined precision / recall.

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
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {10}, f64);

  const lymph::ClassReport zero =
      lymph::metrics::classification::summary(truth, predicted,
                                    lymph::ClassReportOptions{.zero_division = 0.0});
  const lymph::ClassReport one =
      lymph::metrics::classification::summary(truth, predicted,
                                    lymph::ClassReportOptions{.zero_division = 1.0});

  std::cout << "zero_division=0.0  precision = " << zero.precision << "\n";
  std::cout << "zero_division=1.0  precision = " << one.precision << "\n";

  return 0;
}
```

Output

```text
zero_division=0.0  precision = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 0]
zero_division=1.0  precision = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 1]
```

### Example 3

Multiclass — three labels, with the support-weighted average.

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
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 2, 1, 2, 2, 2, 0}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "labels           = " << report.labels << "\n";
  std::cout << "support          = " << report.support << "\n";
  std::cout << "f1 per class     = " << report.f1 << "\n";
  std::cout << "weighted_average = " << report.weighted_average << "\n";

  return 0;
}
```

Output

```text
labels           = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
support          = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 3, 4]
f1 per class     = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.8, 0.66666667, 0.75]
weighted_average = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.75, 0.75, 0.75]
```

## See Also

- [`ClassReport`](ClassReport.md) — the return type's field reference.
- [`confusion_table`](confusion_table.md) — raw counts instead of rates.
- [`classification`](classification.md) — the `metrics::classification` group.
- [`reports`](../reports.md) — the `(model, data)` overload.
