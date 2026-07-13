# `FeaturePickResult`

## API Signature

- `FeaturePickResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FeaturePickResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `FeatureBatch reduced{}`
- `Series selected_feature_indices = Series::undefined()`
- `Series scores = Series::undefined()`
- `Series ranked_feature_indices = Series::undefined()`

## Examples

```cpp
FeaturePickResult cfg{};
```
