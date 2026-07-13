# `SparseCsrMatrix`

## API Signature

- `SparseCsrMatrix`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseCsrMatrix` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<std::int64_t> row_ptr{}`
- `std::vector<std::int64_t> col_ind{}`
- `std::vector<double> values{}`
- `std::int64_t rows = 0`
- `std::int64_t cols = 0`

## Examples

```cpp
SparseCsrMatrix cfg{};
```
