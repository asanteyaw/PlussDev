# `make_classifier`

## API Signature

- `make_classifier(spec: const ModelSpec&) -> StatusOr<std::unique_ptr<ClassifierBase>>`

## Notes

Factory entry point that validates spec/options and returns a ready-to-train component.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<std::unique_ptr<ClassifierBase>>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

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
auto res = make_classifier(/* spec */);
if (!res.ok()) {
  out.error("make_classifier failed", res.status().message);
} else {
  out.info("make_classifier ok");
}
```

**Expected output**

```text
make_classifier: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
                            MetricKind::QuantileLoss,
                            TuneObjectivePolicy::Auto);
  if (!require_ok(reg_sweep_or, "sweep(regression quantile)")) return 1;
  std::cout << "regression quantile sweep objectives:";
  for (const auto& trial : reg_sweep_or.value().trials) std::cout << " " << trial.objective;
  std::cout << "\n";
  std::cout << "regression quantile sweep best index: " << reg_sweep_or.value().best_index << "\n";

  auto bounded_or = make_classifier(linear_logistic_spec());
  if (!require_ok(bounded_or, "make_classifier(linear bounded)")) return 1;
  auto bounded = bounded_or.move_value();

  TrainOptions bounded_train = train;
  bounded_train.parameter_lower_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  bounded_train.parameter_upper_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  if (!bounded->learn(batch, bounded_train).ok()) {
    std::cerr << "bounded linear learn failed\n";
    return 1;
  }

  auto bounded_scores_or = bounded->infer_scores(batch.features);
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
void print_metric(const std::string& label, const Series& m) {
  const auto v = m.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << label << ":";
  for (double x : v) std::cout << " " << x;
  std::cout << "\n";
}

void run_classifier_family(const std::string& name, const ModelSpec& spec, const LabeledBatch& train) {
  auto model_or = make_classifier(spec);
  if (!require_ok(model_or, name + " make_classifier")) return;
  std::unique_ptr<ClassifierBase> model = model_or.move_value();
  if (!require_status(model->learn(train), name + " learn")) return;

  auto pred_or = model->infer(train.features);
  if (!require_ok(pred_or, name + " infer")) return;
  auto metric_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
  if (!require_ok(metric_or, name + " assess")) return;
  print_metric("  " + name + " (acc, logloss)", metric_or.value());
}

void run_regressor_family(const std::string& name, const ModelSpec& spec, const LabeledBatch& train) {
```

**Expected output**

```text
NodeJ model families demo
Classification families:
  linear (acc, logloss): 1 4.47877e-06
  linear_hinge (acc, logloss): 1 0.208572
  tree (acc, logloss): 1 9.99978e-13
  ensemble (acc, logloss): 1 1.00003e-12
  neighbor (acc, logloss): 1 9.99978e-13
  kernel (acc, logloss): 1 0.318438
  bayes (acc, logloss): 1 9.99978e-13
Regression families:
  linear (mse, r2): 1.52381e-19 1
  tree (mse, r2): 0 1
...
```
