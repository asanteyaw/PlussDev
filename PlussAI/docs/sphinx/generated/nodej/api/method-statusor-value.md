# `StatusOr::value`

## API Signature

- `StatusOr::value() -> const T&`
- `StatusOr::value() -> T&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `T&`.
- Returns `const T&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = StatusOr::value();
out.info("value", value);
```

**Expected output**

```text
value returned const T&
```

### Example 2 (Demo Pattern)

```cpp
      MetricKind::Precision,
      MetricKind::Recall,
      MetricKind::F1,
      MetricKind::RocAuc,
      MetricKind::LogLoss};

  auto linear_cv_or = cross_assess(linear_logistic_spec(), batch, split, train, assess);
  if (!require_ok(linear_cv_or, "cross_assess(linear)")) return 1;
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
```

**Expected output**

```text
NodeJ CV + tune demo
linear mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0 0 0 0 0 0 1.12262
linear std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0 0 0 0 0 0 0.169779
kernel mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 1 1 1 1 1 1 0.420312
kernel std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0 0 0 0 0 0 0.0248626
sweep trial objectives: 0 1
sweep best index: 1
probe best index: 1
regression quantile sweep objectives: 0.0352983 0.743786
regression quantile sweep best index: 0
max |bounded score|: 0
NodeJ CV + tune demo completed.
```

### Example 3 (Demo Pattern)

```cpp
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.note_policy = DataNotePolicy::Hash;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;

  auto batch_or = labeled_batch_from_dataframe(frame, options);
  if (!require_ok(batch_or, "labeled_batch_from_dataframe")) return 1;

  const LabeledBatch& batch = batch_or.value();
  std::cout << "adapted batch: samples=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count()
            << " weighted=" << (batch.sample_weights.defined() ? "yes" : "no") << "\n";

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.5165e-17 1
NodeJ DataFrame bridge demo completed.
```
