# `DependenceSurface`

## API Signature

- `DependenceSurface`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `DependenceSurface` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::int64_t feature_a = -1`
- `std::int64_t feature_b = -1`
- `Series grid_a = Series::undefined()`
- `Series grid_b = Series::undefined()`
- `Series mean_prediction = Series::undefined()`

## Examples

```cpp
DependenceSurface cfg{};
// cfg.feature_a = ...;
```
