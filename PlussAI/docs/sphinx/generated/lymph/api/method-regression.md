# `regression`

## API Signature

- `regression(truth: Series, prediction: Series) -> Regression`

## Notes

Namespace-level API function.

## Inputs

- `prediction` (`Series`)
  Pluss numeric vector/tensor input.
- `truth` (`Series`)
  Target values aligned to input observations.

## Output

- Returns `Regression`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = regression(/* truth */, /* prediction */);
out.info("regression", value);
```

**Expected output**

```text
regression returned Regression
```

### Example 2 (Demo Pattern)

```cpp
  const std::vector<ModelSpec> reg_candidates{linear_reg_spec(1e-9), linear_reg_spec(200.0)};
  auto reg_sweep_or = sweep(reg_candidates,
                            reg_batch,
                            split,
                            TrainOptions{},
                            reg_assess,
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
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-8,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(reg_spec);
    if (!require_ok(flow_or, "make_flow(regression)")) return 1;
    Flow reg = flow_or.move_value();
    if (!reg.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) return 1;
    if (!reg.add_standard_scaler().ok()) return 1;

    if (!reg.learn(train).ok()) {
      std::cerr << "regression flow learn failed\n";
      return 1;
    }

    auto metric_or = reg.assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!require_ok(metric_or, "regression flow assess")) return 1;
    print_series("regression flow metrics [mse, r2]", metric_or.value());
```

**Expected output**

```text
NodeJ Flow demo
classification flow metrics [accuracy, logloss]: 1 1.44251e-05
regression flow metrics [mse, r2]: 4.46559e-17 1
NodeJ Flow demo completed.
```
