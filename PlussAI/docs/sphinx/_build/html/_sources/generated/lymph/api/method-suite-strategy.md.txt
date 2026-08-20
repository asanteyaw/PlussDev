# `Suite::strategy`

## API Signature

- `Suite::strategy(name: std::string, balance: BalanceTarget) -> Suite&`
- `Suite::strategy(name: std::string, balance: BalanceTarget, options: TrainOptions) -> Suite&`

## Notes

Instance method on `self`.

## Inputs

- `balance` (`BalanceTarget`)
  Method argument; see signature type for accepted form.
- `name` (`std::string`)
  Method argument; see signature type for accepted form.
- `options` (`TrainOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Suite&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Suite::strategy(/* name */, /* balance */);
out.info("strategy", value);
```

**Expected output**

```text
strategy returned Suite&
```

### Example 2 (Demo Pattern)

```cpp
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(cls_spec);
    if (!require_ok(flow_or, "make_flow(classification)")) return 1;
    Flow cls = flow_or.move_value();
    if (!cls.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) return 1;
    if (!cls.add_standard_scaler().ok()) return 1;

    TrainOptions opts{};
    opts.max_iterations = 3000;
    opts.tolerance = 1e-8;
    if (!cls.learn(train, opts).ok()) {
      std::cerr << "classification flow learn failed\n";
      return 1;
    }

    auto metric_or = cls.assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!require_ok(metric_or, "classification flow assess")) return 1;
```

**Expected output**

```text
NodeJ Flow demo
classification flow metrics [accuracy, logloss]: 1 1.44251e-05
regression flow metrics [mse, r2]: 4.46559e-17 1
NodeJ Flow demo completed.
```

### Example 3 (Demo Pattern)

```cpp
            << " test idx count=" << strat_or.value().test_indices.numel() << "\n";

  split.folds = 3;
  auto fold_or = pluss::ai::nodej::crossval_plan(dense.sample_count(), split);
  if (!require_ok(fold_or, "crossval_plan")) return 1;
  std::cout << "kfold folds=" << fold_or.value().folds.size() << "\n";

  SimpleImputer imputer(SimpleImputerOptions{
      .strategy = SimpleImputerStrategy::Median,
      .constant_value = 0.0,
      .empty_feature_fallback = 0.0,
  });
  auto imp_or = imputer.learn_map(dense);
  if (!require_ok(imp_or, "SimpleImputer.learn_map")) return 1;
  const Series x_imputed = imp_or.move_value();
  print_shape("imputed", x_imputed);

  StandardScaler zscore{};
  auto z_or = zscore.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(z_or, "StandardScaler.learn_map")) return 1;
  const Series x_z = z_or.move_value();
```

**Expected output**

```text
NodeJ split + prep demo
holdout train idx count=4 test idx count=2
strat holdout train idx count=4 test idx count=2
kfold folds=3
imputed shape=[6, 3]
standardized shape=[6, 3]
minmax shape=[6, 3]
robust shape=[6, 3]
neighbor-imputed shape=[6, 3]
iterative-imputed shape=[6, 3]
vector-normalized shape=[6, 3]
maxabs shape=[6, 3]
...
```
