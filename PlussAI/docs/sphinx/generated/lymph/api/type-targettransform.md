# `TargetTransform`

## API Signature

- `TargetTransform`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TargetTransform` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name = "target_transform"`
- `bool check_inverse = true`
- `double check_inverse_tolerance = 1e-3`

## Examples

```cpp
TargetTransform cfg{};
// cfg.name = ...;
```
