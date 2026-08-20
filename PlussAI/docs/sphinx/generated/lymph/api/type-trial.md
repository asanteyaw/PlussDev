# `Trial`

## API Signature

- `Trial`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Trial` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `double objective = std::numeric_limits<double>::quiet_NaN()`
- `metrics::CrossValidationReport cv{}`

## Examples

```cpp
Trial cfg{};
```
