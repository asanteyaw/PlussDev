# `FoldBatchPair`

## API Signature

- `FoldBatchPair`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FoldBatchPair` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `LabeledBatch train{}`
- `LabeledBatch test{}`

## Examples

```cpp
FoldBatchPair cfg{};
```
