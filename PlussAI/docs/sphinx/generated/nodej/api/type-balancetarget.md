# `BalanceTarget`

## API Signature

- `BalanceTarget`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `BalanceTarget` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `BalanceTargetKind kind = BalanceTargetKind::Keep`
- `double ratio = 1.0`
- `Series counts = Series::undefined()`

## Examples

```cpp
BalanceTarget cfg{};
// cfg.kind = ...;
```
