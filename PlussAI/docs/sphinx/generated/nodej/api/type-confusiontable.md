# `ConfusionTable`

## API Signature

- `ConfusionTable`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `ConfusionTable` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series labels = Series::undefined()`
- `Series counts = Series::undefined()`
- `ConfusionNormalize normalize = ConfusionNormalize::None`

## Examples

```cpp
ConfusionTable cfg{};
// cfg.labels = ...;
```
