# `DependenceCurve`

## API Signature

- `DependenceCurve`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `DependenceCurve` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::int64_t feature_index = -1`
- `Series grid = Series::undefined()`
- `Series mean_prediction = Series::undefined()`

## Examples

```cpp
DependenceCurve cfg{};
// cfg.feature_index = ...;
```
