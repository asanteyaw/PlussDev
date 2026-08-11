# `Workflow::prep`

## API Signature

- `Workflow::prep() -> ModelPrep`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `ModelPrep`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::prep();
out.info("prep", value);
```

**Expected output**

```text
prep returned ModelPrep
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
