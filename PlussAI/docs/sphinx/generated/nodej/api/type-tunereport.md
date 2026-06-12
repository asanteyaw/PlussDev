# `TuneReport`

## API Signature

- `TuneReport`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `TuneReport` object used by NodeJ/Lymph workflows.

## Notes

Structured report object carrying metrics/diagnostics from training/evaluation routines.

## Fields

Public fields on this type:

- `std::vector<TuneTrialResult> trials{}`
- `std::int64_t best_index = -1`
- `MetricKind objective_metric = MetricKind::Accuracy`
- `TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto`
- `std::vector<std::int64_t> objective_rank{}`
- `TuneLedger ledger{}`

## Examples

### Example 1

```cpp
      train_options,
      assess_options,
      MetricKind::Rmse,
      TuneObjectivePolicy::Minimize);
  if (!broad_sweep_or.ok()) {
    std::cerr << "sweep(broad) failed: " << broad_sweep_or.status().message << "\n";
    return 1;
  }
  TuneReport broad_sweep = broad_sweep_or.move_value();

  std::vector<std::size_t> broad_success_indices{};
  for (std::size_t i = 0; i < broad_sweep.trials.size(); ++i) {
    if (!broad_sweep.trials[i].status.ok()) continue;
    if (!std::isfinite(broad_sweep.trials[i].objective)) continue;
    broad_success_indices.push_back(i);
  }
  if (broad_success_indices.empty()) {
    std::cerr << "broad sweep produced no successful trials.\n";
    return 1;
  }
```

**Expected output**

```text
TuneReport configured
```
