# `WeightPlan`

## API Signature

- `WeightPlan`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `WeightPlan` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series weights = Series::undefined()`
- `Series class_weights = Series::undefined()`
- `Series sample_weights = Series::undefined()`
- `ClassMix mix{}`
- `double mean_weight = 1.0`

## Examples

```cpp
WeightPlan cfg{};
// cfg.weights = ...;
```
