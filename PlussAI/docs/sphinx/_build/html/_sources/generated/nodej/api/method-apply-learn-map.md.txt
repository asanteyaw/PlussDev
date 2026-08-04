# `apply_learn_map`

## API Signature

- `apply_learn_map(batch: const FeatureBatch&, learn_options: const TrainOptions&, infer_options: const InferOptions&) -> StatusOr<FeatureBatch>`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `infer_options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `learn_options` (`const TrainOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<FeatureBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `FeatureBatch` (`struct`):
  - `Series x = Series::undefined()`
    Option field used by this method.
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.

- `InferOptions` (`struct`):
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.
  - `MissingPolicy missing = MissingPolicy::Reject`
    Option field used by this method.
  - `UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error`
    Controls fold/trial/count sizing.
  - `double positive_threshold = 0.5`
    Option field used by this method.

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
  - `BalanceTarget balance{}`
    Learning-rate step size.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = apply_learn_map(/* batch */, /* learn_options */, /* infer_options */);
if (!res.ok()) {
  out.error("apply_learn_map failed", res.status().message);
} else {
  out.info("apply_learn_map ok");
}
```

**Expected output**

```text
apply_learn_map: ok
status.ok() == true
result.value() is available
```
