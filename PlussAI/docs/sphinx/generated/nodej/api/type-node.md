# `Node`

## API Signature

- `Node`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Node` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::int32_t point_index = -1`
- `std::int32_t axis = -1`
- `std::int32_t left = -1`
- `std::int32_t right = -1`

## Examples

```cpp
Node cfg{};
// cfg.point_index = ...;
```
