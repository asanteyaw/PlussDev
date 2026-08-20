# `search`

## API Signature

- `search`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `search` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `selection::CompareReport report{}`
- `std::optional<Model> best_model{}`
- `std::vector<Model> fitted{}`

## Examples

```cpp
search cfg{};
```
