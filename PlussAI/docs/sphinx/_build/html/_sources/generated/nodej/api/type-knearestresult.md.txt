# `KNearestResult`

## API Signature

- `KNearestResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `KNearestResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series indices = Series::undefined()`
- `Series distances2 = Series::undefined()`

## Examples

```cpp
KNearestResult cfg{};
// cfg.indices = ...;
```
