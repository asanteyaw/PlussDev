# `LinearParameters`

## API Signature

- `LinearParameters`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `LinearParameters` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series coeff = Series::undefined()`
- `Series intercept = Series::undefined()`
- `bool fit_intercept = false`

## Examples

```cpp
LinearParameters cfg{};
// cfg.coeff = ...;
```
