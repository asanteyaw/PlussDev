# `FlowGraphInfo`

## API Signature

- `FlowGraphInfo`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FlowGraphInfo` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `TaskKind task = TaskKind::Transform`
- `bool trained = false`
- `bool has_model = false`
- `bool has_opaque_transforms = false`
- `std::vector<FlowStageInfo> stages{}`

## Examples

```cpp
FlowGraphInfo cfg{};
// cfg.task = ...;
```
