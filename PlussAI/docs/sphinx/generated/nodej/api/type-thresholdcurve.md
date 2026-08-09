# `ThresholdCurve`

## API Signature

- `ThresholdCurve`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `ThresholdCurve` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series thresholds = Series::undefined()`
- `Series precision = Series::undefined()`
- `Series recall = Series::undefined()`
- `Series f1 = Series::undefined()`

## Examples

```cpp
ThresholdCurve cfg{};
// cfg.thresholds = ...;
```
