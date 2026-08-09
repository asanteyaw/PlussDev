# `CompareResult`

## API Signature

- `CompareResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CompareResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `CompareReport report{}`
- `std::optional<Model> best_model{}`
- `std::vector<Model> fitted{}`

## Examples

```cpp
CompareResult cfg{};
```
