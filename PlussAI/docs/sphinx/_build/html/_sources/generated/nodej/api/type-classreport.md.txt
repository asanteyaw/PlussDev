# `ClassReport`

## API Signature

- `ClassReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `ClassReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `Series labels = Series::undefined()`
- `Series support = Series::undefined()`
- `Series precision = Series::undefined()`
- `Series recall = Series::undefined()`
- `Series f1 = Series::undefined()`
- `Series accuracy = Series::undefined()`
- `Series micro_average = Series::undefined()`
- `Series macro_average = Series::undefined()`
- `Series weighted_average = Series::undefined()`

## Examples

```cpp
ClassReport cfg{};
// cfg.labels = ...;
```
