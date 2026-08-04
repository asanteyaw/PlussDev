# `BatchHoldoutSplit`

## API Signature

- `BatchHoldoutSplit`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `BatchHoldoutSplit` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `LabeledBatch train_set{}`
- `LabeledBatch test_set{}`
- `IndexSplit indices{}`

## Examples

```cpp
BatchHoldoutSplit cfg{};
```
