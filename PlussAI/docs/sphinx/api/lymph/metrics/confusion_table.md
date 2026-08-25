# `lymph::confusion_table`

## Summary

`confusion_table(...)` returns a class-by-class count table for hard-label
classification results.

Use it when you want a compact diagnostic of where a classifier is confusing one
class for another.

## Why It Matters

A single scalar metric can hide important failure modes.

Two models can have similar accuracy while making very different mistakes.

`confusion_table(...)` answers the more concrete question:

"when the truth is class A, what does the model actually predict?"

That makes it one of the fastest ways to inspect class-specific error behavior.

## What It Shows

The table organizes counts by:

- true class
- predicted class

From that, you can immediately see:

- which classes are being confused
- whether one class is being over-predicted
- whether some class is barely being recognized at all

## Option Surface

The lower-level options are carried through `ConfusionOptions`:

```cpp
lymph::ConfusionOptions{
    .normalize = lymph::ConfusionNormalize::None,
    .label_order = Series::undefined(),
    .sample_weights = Series::undefined(),
};
```

### `normalize`

Controls whether raw counts are returned or the table is normalized.

Common modes include:

- `None`
- `ByTruth`
- `ByPrediction`
- `Global`

Use:

- `None` when you want raw event counts
- `ByTruth` when you want row-wise class recall style interpretation
- `ByPrediction` when you want column-wise precision style interpretation
- `Global` when you want one globally normalized mass view

### `label_order`

Pins the class ordering explicitly.

Use it when:

- you want a stable presentation order
- some classes may be absent in one evaluation slice but should still be ordered
  consistently

### `sample_weights`

Lets the confusion table reflect weighted observations rather than plain unit
counts.

Use this when the evaluation contract itself is weighted and the diagnostic should
match that contract honestly.

## When To Use It

Use confusion tables:

- after a promising validation result
- during model comparison for imbalanced classification
- before choosing a production threshold
- whenever business error types matter more than a single scalar summary

## Signature

```cpp
lymph::ConfusionTable lymph::confusion_table(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    lymph::ConfusionOptions options = {});
lymph::ConfusionTable lymph::reports::confusion_table(...) ;
```

## Practical Example

This example is fully specified on purpose:

- it creates the truth labels
- it creates the predicted labels
- it prints the resulting object explicitly with `std::cout`

That way there is no hidden state for you to guess about.

```cpp
#include <pluss/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  const pluss::Series truth_labels = pluss::make_series({0.0, 1.0});
  const pluss::Series predicted_labels = pluss::make_series({0.0, 1.0});

  const lymph::ConfusionTable table =
      lymph::confusion_table(
          truth_labels,
          predicted_labels,
          lymph::ConfusionOptions{
              .normalize = lymph::ConfusionNormalize::ByTruth,
          });

  std::cout << "== confusion_table ==\n";
  std::cout << "labels=" << table.labels << "\n";
  std::cout << "counts=" << table.counts << "\n";
  std::cout << "normalize=" << static_cast<int>(table.normalize) << "\n";
}
```

### Output

```text
== confusion_table ==
labels=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
counts=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0],
  [0, 1]]
normalize=1
```

This is the real terminal output from a verifier example. Read it closely.

`ConfusionTable` is not a decorative pretty-printer. It is a real object with:

- `labels` for explicit class order
- `counts` as a matrix-shaped `Series`
- `normalize` telling you how to interpret the matrix numerically

If you already have a fitted model, the same diagnostic can be obtained from the
reporting surface instead of from raw label vectors:

```cpp
lymph::ConfusionTable table =
    lymph::reports(model).confusion_table(batch);
```

That second path is convenient later in a workflow, but the direct form above is
the one to learn first because it makes the data contract obvious.

### Truth-normalized view

```cpp
lymph::ConfusionTable normalized =
    lymph::confusion_table(
        truth_labels,
        predicted_labels,
        {
            .normalize = lymph::ConfusionNormalize::ByTruth,
        });
```

This is especially useful when class frequencies differ a lot and raw counts alone
would be misleading.

## Related Pages

- [classification_summary](classification_summary.md)
- [CrossValidationReport](CrossValidationReport.md)
- [Threshold](Threshold.md)
