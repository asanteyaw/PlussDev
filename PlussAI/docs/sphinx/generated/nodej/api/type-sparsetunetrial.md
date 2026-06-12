# `SparseTuneTrial`

## API Signature

- `SparseTuneTrial`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseTuneTrial` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `LinearFamilyOptions options{}`
- `Status status = Status::success()`
- `SparseCrossReport assess{}`
- `double objective = std::numeric_limits<double>::quiet_NaN()`

## Examples

```cpp
SparseTuneTrial cfg{};
```
