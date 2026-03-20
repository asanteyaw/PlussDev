# `FrameHoldoutSplit`

## API Signature

- `FrameHoldoutSplit`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FrameHoldoutSplit` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `pluss::table::DataFrame train_set{}`
- `pluss::table::DataFrame test_set{}`
- `IndexSplit indices{}`

## Examples

```cpp
FrameHoldoutSplit cfg{};
```
