# `pluss::ai::classic::ConfusionTable`

A `[k, k]` confusion matrix — row `i`, column `j` is the (possibly
normalized, possibly weighted) count of rows whose true class is
`labels[i]` and predicted class is `labels[j]`. Every statistic
[`classification_summary`](classification_summary.md) computes is derived
from exactly this matrix.

## Signature

```cpp
pluss::Series classic::ConfusionTable::labels;
pluss::Series classic::ConfusionTable::counts;
classic::ConfusionNormalize classic::ConfusionTable::normalize;
```

## Construction

Produced only by [`confusion_table`](confusion_table.md). A plain value
type — copy or store it freely.

## State

`labels` — `pluss::Series`
: `Float64`, rank-1, length `k`. The class values in row/column order —
  either the resolved sorted union of every value seen, or the exact
  [`ConfusionOptions::label_order`](ConfusionOptions.md#label_order) you
  supplied.

`counts` — `pluss::Series`
: `Float64`, shape `[k, k]`. Row `i` is truth class `labels[i]`; column `j`
  is predicted class `labels[j]`. Raw counts, or normalized per
  `normalize` — see [`ConfusionOptions::normalize`](ConfusionOptions.md#normalize)
  for exactly what each mode divides by.

`normalize` — `ConfusionNormalize`
: Which normalization was applied — a copy of whatever
  [`ConfusionOptions::normalize`](ConfusionOptions.md#normalize) the
  producing call was given, kept alongside the counts so a `ConfusionTable`
  is self-describing.

## Methods

None — `ConfusionTable` is a plain data holder, read directly.

## Practical Example

A 3-class confusion matrix, its diagonal (correct predictions), and the
`ByTruth`-normalized version whose diagonal is each class's recall.

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

  const cls::ConfusionTable table = cls::confusion_table(truth, pred).value();
  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts = " << table.counts << "\n";

  cls::ConfusionOptions normalized{};
  normalized.normalize = cls::ConfusionNormalize::ByTruth;
  const cls::ConfusionTable by_truth = cls::confusion_table(truth, pred, normalized).value();
  std::cout << "ByTruth counts (diagonal = per-class recall) = " << by_truth.counts << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
counts = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[2, 1, 0],
  [0, 2, 1],
  [0, 0, 3]]
ByTruth counts (diagonal = per-class recall) = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[0.66666667, 0.33333333, 0],
  [0, 0.66666667, 0.33333333],
  [0, 0, 1]]
```

## Changelog

- **0.1** — `labels`, `counts`, `normalize`.

## See Also

- [`confusion_table`](confusion_table.md) — the producing function.
- [`ConfusionOptions`](ConfusionOptions.md) — the options struct, including
  the `ConfusionNormalize` enum.
- [`ClassReport`](ClassReport.md) — the fuller report built on this matrix.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
