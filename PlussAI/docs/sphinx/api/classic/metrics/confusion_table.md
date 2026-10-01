# `pluss::ai::classic::confusion_table`

Tally a `[k, k]` [`ConfusionTable`](ConfusionTable.md) from truth and
predicted class labels — row `i` is truth class `i`, column `j` is
predicted class `j`. Every function in [`classification_summary`](classification_summary.md)
is built directly on this same computation.

## Signature

```cpp
classic::StatusOr<classic::ConfusionTable> classic::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    const classic::ConfusionOptions& options = {});

classic::StatusOr<classic::ConfusionTable> classic::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    /* a range of double */ label_order,
    classic::ConfusionNormalize normalize = classic::ConfusionNormalize::None);

classic::StatusOr<classic::ConfusionTable> classic::confusion_table(
    const classic::ClassifierBase& classifier,
    const classic::LabeledBatch& batch,
    const classic::InferOptions& infer_options = {},
    const classic::ConfusionOptions& options = {});
```

## What It Does

Counts, for every `(truth class, predicted class)` pair, how many rows
fall into that cell, and lays the counts out as a `[k, k]` matrix -- the
raw tally every derived classification statistic in this family (accuracy,
precision, recall, F1, ...) is computed from. The third overload runs
inference from a classifier and batch internally first, so the raw
predictions never need to be computed by hand.

The second overload accepts anything convertible to a range of `double`
(including a plain `{...}` brace-initializer list of class values) and
forwards it as `ConfusionOptions::label_order` with the given `normalize`.

## Parameters

`truth_labels` / `predicted_labels`
: Rank-any `Series` of class values, one per row (reshaped to rank-1
  internally) — equal length, both finite (no `NaN`/`Inf`), both non-empty.

`options` (`ConfusionOptions`, default `{}`)
: `normalize`, `label_order`, `sample_weights` — see
  [`ConfusionOptions`](ConfusionOptions.md) for every field.

`label_order` / `normalize` (second overload)
: The same two fields as above, spelled as direct parameters for the common
  case where you want to fix the class order without weighting rows or
  building a `ConfusionOptions` by hand.

`classifier` / `batch` / `infer_options` (third overload)
: Runs `classifier.infer(batch.features, infer_options)` first, then scores
  `batch.y` against that prediction — the model-attached convenience form.
  `batch` must pass [`validate_labeled`](../data/LabeledBatch.md).

## Returns

A `StatusOr<ConfusionTable>` — see [`ConfusionTable`](ConfusionTable.md) for
its fields.

## Errors

- Either label vector undefined, empty, length-mismatched, or containing
  non-finite values — `Status::invalid_argument` / `Status::shape_mismatch`.
- `label_order` (explicit or supplied) contains duplicates, or is empty when
  explicitly given — `Status::invalid_argument`.
- `truth_labels` or `predicted_labels` contains a value absent from the
  resolved `label_order` — `Status::invalid_argument` (only reachable when
  `label_order` was supplied explicitly; the default sorted-union order
  always contains every value seen).
- `sample_weights` wrong length, non-finite, or negative —
  `Status::shape_mismatch` / `Status::invalid_argument`.
- Third overload: `batch` fails label-batch validation, or `infer` itself
  fails — that `Status`, propagated.

## Practical Examples

The default sorted-union label order, an explicit `label_order` that
reserves a row/column for a class absent from this call's data, the
classifier-attached overload, and `ConfusionOptions::sample_weights`
removing one row's contribution from the matrix entirely.

### Example 1

Two classes, discovered from the data — the default behavior.

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

  const cls::ConfusionTable table = cls::confusion_table(truth, pred).value();
  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts = " << table.counts << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
counts = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [0, 2]]
```

### Example 2

An explicit `label_order` naming a third class (`2.0`) that never appears in
this particular batch — it still gets an all-zero row and column, rather
than being silently dropped.

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

  const cls::ConfusionTable table = cls::confusion_table(truth, pred, {0.0, 1.0, 2.0}).value();
  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts = " << table.counts << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
counts = pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 1, 0],
  [0, 2, 0],
  [0, 0, 0]]
```

### Example 3

The classifier-attached overload — no separate `infer` call needed.

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

  const cls::ConfusionTable table = cls::confusion_table(*made.value(), batch).value();
  std::cout << "counts = " << table.counts << "\n";

  return 0;
}
```

Output

```text
counts = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 0],
  [0, 4]]
```

### Example 4

`ConfusionOptions::sample_weights`: the same truth/pred pair as Example 1,
but row `1` (`truth=0`, predicted `1`) is given weight `0` — its
contribution to the matrix disappears entirely, as if the row were never
there.

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

  cls::ConfusionOptions options{};
  options.sample_weights = pluss::make_series({1.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::ConfusionTable table = cls::confusion_table(truth, pred, options).value();
  std::cout << "counts = " << table.counts << "\n";

  return 0;
}
```

Output

```text
counts = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 2]]
```

## Changelog

- **0.1** — three overloads (`ConfusionOptions`, `label_order`+`normalize`,
  classifier-attached).

## See Also

- [`ConfusionTable`](ConfusionTable.md) / [`ConfusionOptions`](ConfusionOptions.md) —
  the returned type and its options.
- [`classification_summary`](classification_summary.md) — the fuller report
  built on this same matrix.
