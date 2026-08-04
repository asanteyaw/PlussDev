# `LexiconMapperState`

## API Signature

- `LexiconMapperState`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `LexiconMapperState` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `LexiconMapOptions options{}`
- `bool learned = false`
- `std::int64_t learned_document_count = 0`
- `std::vector<std::string> vocabulary{}`
- `std::vector<std::int64_t> document_frequency{}`

## Examples

```cpp
LexiconMapperState cfg{};
```
