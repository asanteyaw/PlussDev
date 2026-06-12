# `TuneTrialResult`

## API Signature

- `TuneTrialResult`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TuneTrialResult` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `ModelSpec spec{}`
- `Status status = Status::success()`
- `CrossAssessReport assess{}`
- `double objective = std::numeric_limits<double>::quiet_NaN()`
- `std::int64_t stage = 0`

## Examples

```cpp
TuneTrialResult cfg{};
```
