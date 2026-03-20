# `FlowBundleMetadata`

## API Signature

- `FlowBundleMetadata`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FlowBundleMetadata` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::uint32_t schema_version = k_flow_bundle_schema`
- `std::string producer = "pluss.ai.nodej"`
- `std::string producer_version = "0.1.0"`
- `std::uint64_t created_unix_nanos = 0`

## Examples

```cpp
FlowBundleMetadata cfg{};
// cfg.schema_version = ...;
```
