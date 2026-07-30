# `MetricReport`

## API Signature

- `MetricReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `MetricReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<MetricKind> requested{}`
- `Series values = Series::undefined()`
- `Series accuracy = Series::undefined()`
- `Series balanced_accuracy = Series::undefined()`
- `Series precision = Series::undefined()`
- `Series recall = Series::undefined()`
- `Series sensitivity = Series::undefined()`
- `Series f1 = Series::undefined()`
- `Series specificity = Series::undefined()`
- `Series geometric_mean = Series::undefined()`
- `Series balance_quality = Series::undefined()`
- `Series roc_auc = Series::undefined()`
- `Series logloss = Series::undefined()`
- `Series mse = Series::undefined()`
- `Series rmse = Series::undefined()`
- `Series mae = Series::undefined()`
- `Series r2 = Series::undefined()`
- `Series quantile_loss = Series::undefined()`

## Examples

```cpp
MetricReport cfg{};
```
