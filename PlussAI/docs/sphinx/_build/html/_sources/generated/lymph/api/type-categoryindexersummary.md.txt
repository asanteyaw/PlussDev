# `CategoryIndexerSummary`

## API Signature

- `CategoryIndexerSummary`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CategoryIndexerSummary` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `bool learned = false`
- `std::int64_t feature_count = 0`
- `bool include_unknown_bucket = true`
- `bool one_based = true`
- `std::vector<std::string> feature_names{}`
- `std::vector<std::string> mapped_names{}`
- `std::vector<std::vector<std::string>> categories{}`

## Examples

```cpp
CategoryIndexerSummary cfg{};
// cfg.learned = ...;
```
