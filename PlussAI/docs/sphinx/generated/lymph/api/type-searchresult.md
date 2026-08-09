# `SearchResult`

## API Signature

- `SearchResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SearchResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `selection::CompareReport report{}`
- `std::optional<Model> best_model{}`
- `std::vector<Model> fitted{}`

## Examples

```cpp
SearchResult cfg{};
```
