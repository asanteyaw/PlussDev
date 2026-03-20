# `StandardScalerSummary`

## API Signature

- `StandardScalerSummary`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `StandardScalerSummary` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<std::string> feature_names{}`
- `Series mean = Series::undefined()`
- `Series scale = Series::undefined()`

## Examples

```cpp
StandardScalerSummary cfg{};
```
