# `BinaryScoreCurve`

## API Signature

- `BinaryScoreCurve`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `BinaryScoreCurve` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series score_sorted = Series::undefined()`
- `Series true_positive_cumulative = Series::undefined()`
- `Series false_positive_cumulative = Series::undefined()`
- `Series is_threshold_boundary = Series::undefined()`
- `Series positive_count = Series::undefined()`
- `Series negative_count = Series::undefined()`

## Examples

```cpp
BinaryScoreCurve cfg{};
// cfg.score_sorted = ...;
```
