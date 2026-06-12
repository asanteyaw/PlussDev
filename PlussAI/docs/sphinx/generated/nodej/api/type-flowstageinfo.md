# `FlowStageInfo`

## API Signature

- `FlowStageInfo`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FlowStageInfo` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::int64_t index = 0`
- `std::string stage_key{}`
- `TransformKind kind = TransformKind::StandardScaler`
- `std::int64_t routed_feature_count = 0`
- `RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough`
- `bool route_active = false`

## Examples

```cpp
FlowStageInfo cfg{};
// cfg.index = ...;
```
