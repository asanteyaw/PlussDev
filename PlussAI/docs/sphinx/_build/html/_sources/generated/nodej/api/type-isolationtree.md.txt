# `IsolationTree`

## API Signature

- `IsolationTree`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `IsolationTree` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<std::int32_t> split_feature{}`
- `std::vector<double> split_value{}`
- `std::vector<std::int32_t> left_child{}`
- `std::vector<std::int32_t> right_child{}`
- `std::vector<std::int32_t> node_size{}`
- `std::vector<std::uint8_t> missing_go_left{}`

## Examples

```cpp
IsolationTree cfg{};
```
