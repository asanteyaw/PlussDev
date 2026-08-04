# `BalancePlan`

## API Signature

- `BalancePlan`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `BalancePlan` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series indices = Series::undefined()`
- `Series target_counts = Series::undefined()`
- `Series synthetic_base_indices = Series::undefined()`
- `Series synthetic_neighbor_indices = Series::undefined()`
- `Series synthetic_steps = Series::undefined()`
- `Series synthetic_labels = Series::undefined()`
- `Series removed_indices = Series::undefined()`
- `ClassMix before{}`
- `ClassMix after{}`
- `bool uses_replacement = false`
- `bool uses_synthesis = false`
- `bool uses_cleaning = false`

## Examples

```cpp
BalancePlan cfg{};
// cfg.indices = ...;
```
