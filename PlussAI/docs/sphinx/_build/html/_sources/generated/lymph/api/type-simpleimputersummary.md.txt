# `SimpleImputerSummary`

## API Signature

- `SimpleImputerSummary`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SimpleImputerSummary` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean`
- `bool learned = false`
- `std::int64_t feature_count = 0`
- `double constant_value = 0.0`
- `double empty_feature_fallback = 0.0`
- `std::vector<std::string> feature_names{}`
- `Series statistics = Series::undefined()`

## Examples

```cpp
SimpleImputerSummary cfg{};
// cfg.strategy = ...;
```
