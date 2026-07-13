# `FeatureRankReport`

## API Signature

- `FeatureRankReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FeatureRankReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `Series scores = Series::undefined()`
- `Series ranked_feature_indices = Series::undefined()`

## Examples

```cpp
FeatureRankReport cfg{};
// cfg.scores = ...;
```
