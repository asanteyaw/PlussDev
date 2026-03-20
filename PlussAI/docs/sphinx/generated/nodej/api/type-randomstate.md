# `RandomState`

## API Signature

- `RandomState`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `RandomState` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::uint64_t seed = 0`
- `bool deterministic = true`

## Examples

```cpp
RandomState cfg{};
// cfg.seed = ...;
```
