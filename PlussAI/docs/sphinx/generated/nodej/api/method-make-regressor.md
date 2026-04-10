# `make_regressor`

## API Signature

- `make_regressor(spec: const ModelSpec&) -> StatusOr<std::unique_ptr<RegressorBase>>`

## Notes

Factory entry point that validates spec/options and returns a ready-to-train component.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<std::unique_ptr<RegressorBase>>`. Check `.ok()` first; on success read `.value()` for the computed result.

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
auto res = make_regressor(/* spec */);
if (!res.ok()) {
  out.error("make_regressor failed", res.status().message);
} else {
  out.info("make_regressor ok");
}
```

**Expected output**

```text
make_regressor: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  };

  auto reg_or = make_regressor(spec);
  if (!require_ok(reg_or, "make_regressor")) return 1;
  auto reg = reg_or.move_value();
  Status st = reg->learn(batch);
  if (!st.ok()) {
    std::cerr << "learn failed: " << st.message << "\n";
    return 1;
  }

  auto metrics_or = reg->assess(batch, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!require_ok(metrics_or, "assess")) return 1;
  print_metrics("linear metrics [mse, r2]", metrics_or.value());
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.5165e-17 1
NodeJ DataFrame bridge demo completed.
```

### Example 3 (Demo Pattern)

```cpp
  auto pred_or = model->infer(train.features);
  if (!require_ok(pred_or, name + " infer")) return;
  auto metric_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
  if (!require_ok(metric_or, name + " assess")) return;
  print_metric("  " + name + " (acc, logloss)", metric_or.value());
}

void run_regressor_family(const std::string& name, const ModelSpec& spec, const LabeledBatch& train) {
  auto model_or = make_regressor(spec);
  if (!require_ok(model_or, name + " make_regressor")) return;
  std::unique_ptr<RegressorBase> model = model_or.move_value();
  if (!require_status(model->learn(train), name + " learn")) return;

  auto pred_or = model->infer(train.features);
  if (!require_ok(pred_or, name + " infer")) return;
  auto metric_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!require_ok(metric_or, name + " assess")) return;
  print_metric("  " + name + " (mse, r2)", metric_or.value());
}

} // namespace
```

**Expected output**

```text
NodeJ model families demo
Classification families:
  linear (acc, logloss): 1 4.47877e-06
  linear_hinge (acc, logloss): 1 0.0886185
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
