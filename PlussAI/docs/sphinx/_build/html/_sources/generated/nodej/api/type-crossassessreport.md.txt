# `CrossAssessReport`

## API Signature

- `CrossAssessReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CrossAssessReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<FoldAssessResult> folds{}`
- `std::vector<MetricKind> metrics{}`
- `Series mean_metrics = Series::undefined()`
- `Series std_metrics = Series::undefined()`

## Examples

```cpp
CrossAssessReport cfg{};
```
