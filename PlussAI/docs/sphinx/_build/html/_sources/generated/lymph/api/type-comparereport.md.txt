# `CompareReport`

## API Signature

- `CompareReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CompareReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<Trial> trials{}`
- `std::int64_t best_index = -1`
- `std::string best_name{}`
- `MetricKind objective_metric = MetricKind::Accuracy`
- `TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto`
- `std::vector<std::int64_t> objective_rank{}`
- `std::vector<std::int64_t> ranked_indices{}`

## Examples

```cpp
CompareReport cfg{};
```
