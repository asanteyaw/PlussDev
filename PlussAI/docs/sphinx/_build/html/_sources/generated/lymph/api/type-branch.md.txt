# `Branch`

## API Signature

- `Branch`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Branch` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<TransformSpec> transforms{}`
- `std::vector<std::string> feature_names{}`
- `std::vector<std::int64_t> feature_route{}`

## Examples

```cpp
Branch cfg{};
```
