# `Builder::has`

## API Signature

- `Builder::has(name: std::string_view) -> bool`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string_view`)
  Method argument; see signature type for accepted form.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::has(/* name */);
out.info("has", value);
```

**Expected output**

```text
has: true
```

### Example 2 (Demo Pattern)

```cpp
    nja::CrossValidationOptions cv_options{};
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    // Was built via the old Workflow{.model=..., .stages={...}} facade --
    // Model already has one builder method per TransformKind
    // (.simple_imputer(...), .standard_scaler(...), ...) that attaches the
    // stage immediately (no lazy replay needed), so this is a direct
    // substitution: same options, called on the model itself before it's
    // handed to a Candidate.
    std::vector<nja::selection::Candidate> candidates{};
    {
      auto wf = nja::linear_factory::linear_regression(LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
```

**Expected output**

```text
CV objective metric: rmse
candidate=linear_ridge objective=102.872 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[102.87158] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.41650024] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[77.611594]
candidate=decision_tree objective=44.9727 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[44.972693] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.88843998] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[27.61723]
candidate=random_forest objective=81.1235 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[81.123538] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.63710967] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
...
```
