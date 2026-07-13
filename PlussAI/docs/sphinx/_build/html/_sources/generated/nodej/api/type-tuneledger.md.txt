# `TuneLedger`

## API Signature

- `TuneLedger`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TuneLedger` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::vector<MetricKind> metrics{}`
- `std::vector<std::string> encoded_param_feature_names{}`
- `Series objective = Series::undefined()`
- `Series objective_rank = Series::undefined()`
- `Series stage = Series::undefined()`
- `Series status_code = Series::undefined()`
- `Series is_success = Series::undefined()`
- `Series mean_test_metrics = Series::undefined()`
- `Series std_test_metrics = Series::undefined()`
- `Series split_test_metrics = Series::undefined()`
- `Series fit_time_seconds = Series::undefined()`
- `Series score_time_seconds = Series::undefined()`
- `Series encoded_params = Series::undefined()`

## Examples

```cpp
TuneLedger cfg{};
```
