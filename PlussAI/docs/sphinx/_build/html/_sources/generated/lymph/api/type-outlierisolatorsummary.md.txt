# `OutlierIsolatorSummary`

## API Signature

- `OutlierIsolatorSummary`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `OutlierIsolatorSummary` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `bool learned = false`
- `std::int64_t feature_count = 0`
- `std::int64_t estimators = 0`
- `std::int64_t max_samples = 0`
- `std::int64_t max_features = 0`
- `bool bootstrap = false`
- `bool contamination_auto = true`
- `double contamination = 0.1`
- `double offset = -0.5`
- `std::vector<std::string> feature_names{}`

## Examples

```cpp
OutlierIsolatorSummary cfg{};
// cfg.learned = ...;
```
