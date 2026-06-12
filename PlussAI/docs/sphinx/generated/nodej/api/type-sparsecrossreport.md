# `SparseCrossReport`

## API Signature

- `SparseCrossReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseCrossReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<SparseFoldReport> folds{}`
- `std::vector<MetricKind> metrics{}`
- `Series mean_metrics = Series::undefined()`
- `Series std_metrics = Series::undefined()`
- `Series mean_train_metrics = Series::undefined()`
- `Series std_train_metrics = Series::undefined()`
- `Series fit_time_seconds = Series::undefined()`
- `Series score_time_seconds = Series::undefined()`

## Examples

```cpp
SparseCrossReport cfg{};
```
