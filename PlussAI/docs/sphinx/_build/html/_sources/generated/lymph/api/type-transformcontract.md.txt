# `TransformContract`

## API Signature

- `TransformContract`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TransformContract` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `bool reversible = false`
- `bool inverse_available = false`
- `bool lossy = true`
- `bool stateful = true`
- `bool target_aware = false`
- `bool changes_sample_count = false`
- `bool changes_feature_count = false`
- `bool needs_inverse_memory = false`

## Examples

```cpp
TransformContract cfg{};
// cfg.reversible = ...;
```
