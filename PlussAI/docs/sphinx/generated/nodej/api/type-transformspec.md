# `TransformSpec`

## API Signature

- `TransformSpec`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TransformSpec` object used by NodeJ/Lymph workflows.

## Notes

Transformer construction contract. Choose a transform kind and configure its options before adding it to a flow/workflow.

## Fields

Public fields on this type:

- `TransformKind kind = TransformKind::StandardScaler`
- `TransformOptions options = prep::StandardScalerOptions{}`
- `std::vector<std::int64_t> feature_route{}`
- `RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough`
- `std::string stage_key{}`

## Examples

```cpp
TransformSpec cfg{};
// cfg.kind = ...;
```
