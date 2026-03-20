# `FoldAssessResult`

## API Signature

- `FoldAssessResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FoldAssessResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series metrics = Series::undefined()`
- `Series train_indices = Series::undefined()`
- `Series test_indices = Series::undefined()`

## Examples

```cpp
FoldAssessResult cfg{};
// cfg.metrics = ...;
```
