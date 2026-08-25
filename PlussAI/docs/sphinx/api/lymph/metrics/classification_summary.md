# `lymph::classification_summary`

## Summary

`classification_summary(...)` returns per-class and aggregate classification
metrics from hard-label predictions.

Use it when you want precision/recall/F1-style diagnostics in one structured
report.

## What It Gives You

`classification_summary(...)` is the richer sibling of a confusion table.

Instead of only raw class-by-class counts, it produces structured diagnostic
metrics such as:

- precision
- recall
- F1
- support
- aggregate views where available

That makes it the right surface when you want not just to see that a model is wrong,
but how it is wrong from a precision/recall perspective.

## Why It Matters

Classification quality is rarely one-dimensional.

For example:

- a fraud model may need high recall
- a churn intervention model may care about precision
- a medical triage model may need a very specific sensitivity profile

`classification_summary(...)` makes those tradeoffs inspectable in one place.

## Typical Use

Use it after:

- validation runs
- final test predictions
- threshold adjustments

It pairs especially well with:

- `confusion_table(...)` for raw class confusion
- `Threshold` for operating-point selection

## Signature

```cpp
lymph::ClassReport lymph::classification_summary(
    const pluss::Series& truth_labels,
    const pluss::Series& predicted_labels,
    lymph::ClassReportOptions options = {});
lymph::ClassReport lymph::reports::classification_summary(...);
```

## Option Surface

The lower-level summary options are carried through `ClassReportOptions`:

```cpp
lymph::ClassReportOptions{
    .label_order = Series::undefined(),
    .zero_division = 0.0,
    .sample_weights = Series::undefined(),
    .balance_quality_alpha = 0.1,
    .square_balance_quality = true,
};
```

### `label_order`

Controls the explicit class presentation order.

Use it when reports need stable ordering across several runs or subsets.

### `zero_division`

Sets the fallback value when a precision or recall denominator is zero.

This is a very important review knob because different fallback conventions can
change how degenerate class situations are communicated.

Use a value in `[0, 1]` to keep the report behavior reference-style and easy to
interpret.

### `sample_weights`

Allows the class report to respect weighted evaluation.

When your scoring contract is weighted, the diagnostic summary should usually be
weighted too.

### `balance_quality_alpha`

Controls the sensitivity of the balance-quality term inside the richer balance
reporting view.

Use smaller or larger values only when you intentionally want to reshape how harshly
imbalance penalties are expressed.

### `square_balance_quality`

Controls whether the balance-quality formulation uses the squared variant.

This is an interpretation toggle rather than a cosmetic one. It changes how sharply
class-imbalance effects are emphasized in the balance-quality summary.

## Practical Example

Like the confusion-table example, this one is intentionally complete.

It shows:

- where the truth labels come from
- where the predicted labels come from
- how the report is printed

```cpp
#include <pluss/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  const pluss::Series truth_labels = pluss::make_series({0.0, 1.0});
  const pluss::Series predicted_labels = pluss::make_series({0.0, 1.0});

  const lymph::ClassReport report =
      lymph::classification_summary(
          truth_labels,
          predicted_labels,
          lymph::ClassReportOptions{
              .zero_division = 0.0,
          });

  std::cout << "== classification_summary ==\n";
  std::cout << "labels=" << report.labels << "\n";
  std::cout << "precision=" << report.precision << "\n";
  std::cout << "recall=" << report.recall << "\n";
  std::cout << "f1=" << report.f1 << "\n";
  std::cout << "accuracy=" << report.accuracy << "\n";
  std::cout << "macro_average=" << report.macro_average << "\n";
}
```

### Output

```text
== classification_summary ==
labels=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
precision=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
recall=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
f1=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
accuracy=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
macro_average=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

This output is copied from a real verifier run.

The concrete lesson is that `ClassReport` gives you both:

- per-class series surfaces such as `precision`, `recall`, and `f1`
- aggregate series surfaces such as `accuracy` and `macro_average`

If you are already on a fitted-model path, the reporting surface can construct the
same object for you:

```cpp
lymph::ClassReport report =
    lymph::reports(model).classification_summary(batch);
```

The report is what you use when a stakeholder asks:

"show me how this classifier behaves per class, not just its overall score."

### Explicit label order and zero-division policy

```cpp
lymph::ClassReport report =
    lymph::classification_summary(
        truth_labels,
        predicted_labels,
        {
            .zero_division = 0.0,
        });
```

This is the style to prefer when the presentation contract itself matters.

## Related Pages

- [confusion_table](confusion_table.md)
- [CrossValidationReport](CrossValidationReport.md)
- [Threshold](Threshold.md)
