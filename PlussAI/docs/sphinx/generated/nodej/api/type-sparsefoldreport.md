# `SparseFoldReport`

## API Signature

- `SparseFoldReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseFoldReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `Series metrics = Series::undefined()`
- `Series train_metrics = Series::undefined()`
- `Series train_indices = Series::undefined()`
- `Series test_indices = Series::undefined()`
- `double fit_time_seconds = 0.0`
- `double score_time_seconds = 0.0`

## Examples

```cpp
SparseFoldReport cfg{};
// cfg.metrics = ...;
```
