# `PermuteImpactReport`

## API Signature

- `PermuteImpactReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `PermuteImpactReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `Series mean_drop = Series::undefined()`
- `Series std_drop = Series::undefined()`
- `Series per_repeat_drops = Series::undefined()`
- `MetricKind metric = MetricKind::Accuracy`

## Examples

```cpp
PermuteImpactReport cfg{};
// cfg.mean_drop = ...;
```
