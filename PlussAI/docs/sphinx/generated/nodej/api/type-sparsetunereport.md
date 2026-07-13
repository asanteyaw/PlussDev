# `SparseTuneReport`

## API Signature

- `SparseTuneReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `SparseTuneReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<SparseTuneTrial> trials{}`
- `std::int64_t best_index = -1`
- `MetricKind objective_metric = MetricKind::Accuracy`
- `SparseObjectivePolicy objective_policy = SparseObjectivePolicy::Auto`
- `std::vector<std::int64_t> objective_rank{}`
- `SparseTuneLedger ledger{}`

## Examples

```cpp
SparseTuneReport cfg{};
```
