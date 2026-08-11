# `Model::Model`

## API Signature

- `Model(arg1: ::pluss::ai::nodej::Workflow workflow) : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(std::move(workflow)))) -> auto`
- `Model(arg1: const ModelSpec& spec) : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(spec))) -> auto`
- `impl_(arg1: pluss::msharp::make_intrusive<detail::ModelImpl>(std::move(core))) -> Model(detail::ModelCore core) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`::pluss::ai::nodej::Workflow workflow) : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(std::move(workflow))) | const ModelSpec& spec) : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(spec)) | pluss::msharp::make_intrusive<detail::ModelImpl>(std::move(core))`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model(detail::ModelCore core) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model(/* arg1 */);
out.info("Model", value);
```

**Expected output**

```text
Model returned result
```

### Example 2 (Demo Pattern)

```cpp
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    std::vector<nja::selection::Candidate> candidates{};
    {
      nja::Model wf = nja::models::linear_regression(LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      candidates.emplace_back("linear_ridge", wf);
    }
    {
      nja::Model wf = nja::models::decision_tree_regressor(TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
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
