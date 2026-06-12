# `VariancePruneResult`

## API Signature

- `VariancePruneResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `VariancePruneResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `FeatureBatch reduced{}`
- `Series selected_feature_indices = Series::undefined()`
- `Series feature_variances = Series::undefined()`

## Examples

```cpp
VariancePruneResult cfg{};
```
