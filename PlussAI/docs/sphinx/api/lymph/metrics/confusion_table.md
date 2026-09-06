<!-- pluss-hand-authored -->

# `lymph::metrics::classification::confusion_table`

[Back to `metrics`](../metrics.md)

`metrics::classification::confusion_table(truth, prediction)` builds a
[`ConfusionTable`](ClassReport.md) from two label `Series` — the truth ×
prediction count matrix, optionally normalized.

## Signature

```cpp
lymph::ConfusionTable lymph::metrics::classification::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const lymph::ConfusionOptions& options = {});

lymph::ConfusionTable lymph::metrics::classification::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    std::initializer_list<double> label_order,
    lymph::ConfusionNormalize normalize = lymph::ConfusionNormalize::None);
```

`lymph::metrics::classification::confusion_table` is the only spelling — the
old bare `lymph::confusion_table` name has been removed.

## What It Does

Discovers the class labels present in either `Series` (or takes them from
`label_order`), then tallies one cell per `(truth, prediction)` pair into a
row-major matrix stored in `ConfusionTable::counts` — element `r*n + c` is
"truth = `labels[r]`, prediction = `labels[c]`". With `normalize` set, the cells
are rates instead of counts:

- `ByTruth` — each row sums to 1; the diagonal is per-class **recall**.
- `ByPrediction` — each column sums to 1; the diagonal is per-class
  **precision**.
- `Global` — the whole matrix sums to 1.

## Parameters

`truth_labels`
: Ground-truth class labels.

`predicted_labels`
: Predicted class labels, same length.

`options` — `lymph::ConfusionOptions`
: `normalize` (default `None`), `label_order` (a `Series` fixing row/column
  order), and `sample_weights`.

`label_order` *(initializer-list overload)*
: The exact row/column order as a `{…}` list — pins classes that may be absent
  from one `Series`.

`normalize` *(initializer-list overload)*
: The `ConfusionNormalize` mode, same meaning as `options.normalize`.

## Returns

A `lymph::ConfusionTable`: `labels`, `counts` (row-major), and the `normalize`
mode it was built with.

## Errors

- Length mismatch between the two `Series` raises.
- A value in `label_order` that appears in neither `Series` still gets a row and
  column (all zero).

## Practical Examples

<!-- these examples establish: raw counts, ByTruth normalization, and a pinned label order. -->

### Example 1

Raw counts.

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

  const lymph::ConfusionTable table =
      lymph::metrics::classification::confusion_table(truth, predicted);

  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts (row-major, truth x pred) = " << table.counts << "\n";
  std::cout << "normalize = " << static_cast<int>(table.normalize) << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
counts (row-major, truth x pred) = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 1],
  [1, 4]]
normalize = 0
```

### Example 2

`ByTruth` normalization turns the rows into per-true-class rates.

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

  const lymph::ConfusionTable raw = lymph::metrics::classification::confusion_table(truth, predicted);
  const lymph::ConfusionTable rates = lymph::metrics::classification::confusion_table(
      truth, predicted,
      lymph::ConfusionOptions{.normalize = lymph::ConfusionNormalize::ByTruth});

  std::cout << "raw counts   = " << raw.counts << "\n";
  std::cout << "row-normed   = " << rates.counts << "\n";
  std::cout << "normalize mode = " << static_cast<int>(rates.normalize) << "\n";

  return 0;
}
```

Output

```text
raw counts   = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 1],
  [1, 4]]
row-normed   = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.8, 0.2],
  [0.2, 0.8]]
normalize mode = 1
```

### Example 3

A pinned label order via the `{…}` overload.

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
      pluss::make_series({2, 2, 0, 0, 1, 1, 2, 0, 1, 2}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({2, 0, 0, 0, 1, 2, 2, 1, 1, 2}, {10}, f64);

  const lymph::ConfusionTable table =
      lymph::metrics::classification::confusion_table(truth, predicted, {2.0, 1.0, 0.0});

  std::cout << "labels (pinned order) = " << table.labels << "\n";
  std::cout << "counts                = " << table.counts << "\n";

  return 0;
}
```

Output

```text
labels (pinned order) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 1, 0]
counts                = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[3, 0, 1],
  [1, 2, 0],
  [0, 1, 2]]
```

## See Also

- [`ClassReport`](ClassReport.md) — the `ConfusionTable` field reference, and
  the per-class rates as a report.
- [`classification_summary`](classification_summary.md) — precision / recall /
  f1 rather than raw counts.
- [`classification`](classification.md) — the `metrics::classification` group.
- [`reports`](../reports.md) — the `(model, data)` overload.
