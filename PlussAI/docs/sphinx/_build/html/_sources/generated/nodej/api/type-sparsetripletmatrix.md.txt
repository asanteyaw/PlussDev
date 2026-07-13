# `SparseTripletMatrix`

## API Signature

- `SparseTripletMatrix`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseTripletMatrix` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series row_indices = Series::undefined()`
- `Series column_indices = Series::undefined()`
- `Series values = Series::undefined()`
- `std::int64_t rows = 0`
- `std::int64_t cols = 0`

## Examples

```cpp
SparseTripletMatrix cfg{};
// cfg.row_indices = ...;
```
