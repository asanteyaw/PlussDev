# `SeriesCompareReport`

## API Signature

- `SeriesCompareReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SeriesCompareReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `Series abs_error = Series::undefined()`
- `Series max_abs_diff = Series::undefined()`
- `Series mae = Series::undefined()`

## Examples

```cpp
SeriesCompareReport cfg{};
// cfg.abs_error = ...;
```
