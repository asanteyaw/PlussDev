# `dependence_surface`

## API Signature

- `dependence_surface(spec: const ModelSpec&, train: const LabeledBatch&, eval: const FeatureBatch&, feature_a: std::int64_t, feature_b: std::int64_t, grid_points_a: std::int64_t = 16, grid_points_b: std::int64_t = 16) -> StatusOr<DependenceSurface>`

## Notes

Namespace-level API function.

## Inputs

- `eval` (`const FeatureBatch&`)
  Method argument; see signature type for accepted form.
- `feature_a` (`std::int64_t`)
  Input feature data used by this call.
- `feature_b` (`std::int64_t`)
  Input feature data used by this call.
- `grid_points_a` (`std::int64_t`), default: `16`
  Method argument; see signature type for accepted form.
- `grid_points_b` (`std::int64_t`), default: `16`
  Method argument; see signature type for accepted form.
- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.
- `train` (`const LabeledBatch&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<DependenceSurface>`. Check `.ok()` first; on success read `.value()` for the computed result.

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
auto res = dependence_surface(/* spec */, /* train */, /* eval */, /* feature_a */, /* feature_b */, /* grid_points_a */, /* grid_points_b */);
if (!res.ok()) {
  out.error("dependence_surface failed", res.status().message);
} else {
  out.info("dependence_surface ok");
}
```

**Expected output**

```text
dependence_surface: ok
status.ok() == true
result.value() is available
```
