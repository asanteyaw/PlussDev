# `WorkflowModel::dependence_curve`

## API Signature

- `WorkflowModel::dependence_curve(train: const LabeledBatch&, eval: const FeatureBatch&, feature_index: std::int64_t, grid_points: std::int64_t = 32) -> DependenceCurve`

## Notes

Instance method on `self`.

## Inputs

- `eval` (`const FeatureBatch&`)
  Method argument; see signature type for accepted form.
- `feature_index` (`std::int64_t`)
  Input feature data used by this call.
- `grid_points` (`std::int64_t`), default: `32`
  Method argument; see signature type for accepted form.
- `train` (`const LabeledBatch&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `DependenceCurve`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::dependence_curve(/* train */, /* eval */, /* feature_index */, /* grid_points */);
out.info("dependence_curve", value);
```

**Expected output**

```text
dependence_curve returned DependenceCurve
```
