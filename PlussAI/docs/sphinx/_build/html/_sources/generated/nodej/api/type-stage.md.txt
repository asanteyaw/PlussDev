# `Stage`

## API Signature

- `Stage`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Stage` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<std::unique_ptr<TransformerBase>> transformers{}`
- `std::vector<std::int64_t> feature_route{}`

## Examples

```cpp
Stage cfg{};
```
