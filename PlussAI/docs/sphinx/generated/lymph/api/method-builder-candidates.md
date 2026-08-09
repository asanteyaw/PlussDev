# `Builder::candidates`

## API Signature

- `Builder::candidates() -> std::vector<selection::Candidate>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<selection::Candidate>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::candidates();
out.info("candidates", value);
```

**Expected output**

```text
candidates returned std::vector<selection::Candidate>
```

### Example 2 (Demo Pattern)

```cpp
  print_series("linear mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", linear_cv_or.value().mean_metrics);
  print_series("linear std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", linear_cv_or.value().std_metrics);

  auto kernel_cv_or = cross_assess(rbf_kernel_spec(), batch, split, train, assess);
  if (!require_ok(kernel_cv_or, "cross_assess(kernel)")) return 1;
  print_series("kernel mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", kernel_cv_or.value().mean_metrics);
  print_series("kernel std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", kernel_cv_or.value().std_metrics);

  const std::vector<ModelSpec> candidates{linear_logistic_spec(), rbf_kernel_spec()};
  AssessOptions tune_assess{};
  tune_assess.metrics = {MetricKind::Accuracy};

  auto sweep_or = sweep(candidates,
                        batch,
                        split,
                        train,
                        tune_assess,
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
  if (!require_ok(sweep_or, "sweep")) return 1;
```

**Expected output**

```text
NodeJ CV + tune demo
linear mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0.4375 0.4375 0.458333 0.625 0.5 0.25 0.773477
linear std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0.108253 0.108253 0.0721688 0.216506 0.117851 0.25 0.0776902
kernel mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 1 1 1 1 1 1 0.372106
kernel std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0 0 0 0 0 0 0.0311667
sweep trial objectives: 0.4375 1
sweep best index: 1
probe best index: 1
regression quantile sweep objectives: 0.0352983 0.743786
regression quantile sweep best index: 0
max |bounded score|: 0
NodeJ CV + tune demo completed.
```

### Example 3 (Demo Pattern)

```cpp
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
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      candidates.emplace_back("linear_ridge", std::move(wf));
    }
    {
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
