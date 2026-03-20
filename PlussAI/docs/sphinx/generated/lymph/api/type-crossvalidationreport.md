# `CrossValidationReport`

## API Signature

- `CrossValidationReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CrossValidationReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `CrossAssessReport report{}`
- `MetricReport mean{}`
- `MetricReport stdev{}`

## Examples

```cpp
CrossValidationReport cfg{};
```
