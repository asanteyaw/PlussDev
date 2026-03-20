# `TransformerSummary`

## API Signature

- `TransformerSummary`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TransformerSummary` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<std::string> feature_names{}`
- `std::vector<Series> state{}`

## Examples

```cpp
TransformerSummary cfg{};
```
