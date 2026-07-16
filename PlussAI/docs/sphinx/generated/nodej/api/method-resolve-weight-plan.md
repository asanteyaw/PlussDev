# `resolve_weight_plan`

## API Signature

- `resolve_weight_plan(labels: const Series&, batch_sample_weights: const Series&, options: const TrainOptions&) -> StatusOr<WeightPlan>`

## Notes

Combines batch sample weights, training sample weights, and class-weight options into a reusable weight plan.

## Inputs

- `batch_sample_weights` (`const Series&`)
  Optional per-row weights aligned to input observations.
- `labels` (`const Series&`)
  Target values aligned to input observations.
- `options` (`const TrainOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<WeightPlan>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `TrainOptions` (`struct`):
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.
  - `MissingPolicy missing = MissingPolicy::Reject`
    Option field used by this method.
  - `UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error`
    Controls fold/trial/count sizing.
  - `std::int64_t max_iterations = 1000`
    Maximum optimization iterations.
  - `double tolerance = 1e-6`
    Convergence tolerance.
  - `bool warm_start = false`
    Option field used by this method.
  - `bool track_diagnostics = true`
    Controls fold/trial/count sizing.
  - `RandomState random{}`
    Random seed for reproducible behavior.
  - `Series sample_weights = Series::undefined()`
    Optional column used as per-row training weight.
  - `ClassWeightMode class_weight = ClassWeightMode::None`
    Option field used by this method.
  - `Series class_weights = Series::undefined()`
    Option field used by this method.
  - `ScoreCalibration calibration = ScoreCalibration::None`
    Option field used by this method.
  - `std::int64_t calibration_iterations = 400`
    Controls fold/trial/count sizing.
  - `double calibration_tolerance = 1e-6`
    Controls fold/trial/count sizing.
  - `Series parameter_lower_bounds = Series::undefined()`
    Option field used by this method.
  - `Series parameter_upper_bounds = Series::undefined()`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = resolve_weight_plan(/* labels */, /* batch_sample_weights */, /* options */);
if (!res.ok()) {
  out.error("resolve_weight_plan failed", res.status().message);
} else {
  out.info("resolve_weight_plan ok");
}
```

**Expected output**

```text
resolve_weight_plan: ok
status.ok() == true
result.value() is available
```
