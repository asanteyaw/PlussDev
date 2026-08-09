# `Stage`

## API Signature

- `Stage`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Stage` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `TransformKind kind = TransformKind::StandardScaler`
- `TransformOptions options = prep::StandardScalerOptions{}`

## Examples

```cpp
Stage cfg{};
// cfg.kind = ...;
```
