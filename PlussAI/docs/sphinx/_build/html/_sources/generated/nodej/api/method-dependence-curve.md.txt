# `dependence_curve`

## API Signature

- `dependence_curve(spec: const ModelSpec&, train: const LabeledBatch&, eval: const FeatureBatch&, feature_index: std::int64_t, grid_points: std::int64_t = 32) -> StatusOr<DependenceCurve>`

## Notes

Namespace-level API function.

## Inputs

- `eval` (`const FeatureBatch&`)
  Method argument; see signature type for accepted form.
- `feature_index` (`std::int64_t`)
  Input feature data used by this call.
- `grid_points` (`std::int64_t`), default: `32`
  Method argument; see signature type for accepted form.
- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.
- `train` (`const LabeledBatch&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<DependenceCurve>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `FeatureBatch` (`struct`):
  - `Series x = Series::undefined()`
    Option field used by this method.
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.

- `LabeledBatch` (`struct`):
  - `FeatureBatch features{}`
    Option field used by this method.
  - `Series y = Series::undefined()`
    Option field used by this method.
  - `Series sample_weights = Series::undefined()`
    Optional column used as per-row training weight.

- `ModelSpec` (`struct`):
  - `TaskKind task = TaskKind::Regression`
    Selects task type (for example classification vs regression).
  - `ModelFamily family = ModelFamily::Linear`
    Selects the model family implementation.
  - `FamilyOptions options = LinearFamilyOptions{}`
    Family-specific options payload used by the selected model/transform family.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = dependence_curve(/* spec */, /* train */, /* eval */, /* feature_index */, /* grid_points */);
if (!res.ok()) {
  out.error("dependence_curve failed", res.status().message);
} else {
  out.info("dependence_curve ok");
}
```

**Expected output**

```text
dependence_curve: ok
status.ok() == true
result.value() is available
```
