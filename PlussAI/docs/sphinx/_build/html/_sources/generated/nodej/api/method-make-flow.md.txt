# `make_flow`

## API Signature

- `make_flow(spec: const ModelSpec&) -> StatusOr<Flow>`

## Notes

Constructs a `Flow` from model spec/options; use it to chain transforms and model training.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<Flow>`. Check `.ok()` first; on success read `.value()` for the computed result.

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
auto res = make_flow(/* spec */);
if (!res.ok()) {
  out.error("make_flow failed", res.status().message);
} else {
  out.info("make_flow ok");
}
```

**Expected output**

```text
make_flow: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };

  auto flow_or = make_flow(spec);
  if (!flow_or.ok()) {
    std::cerr << "make_flow failed: " << flow_or.status().message << "\n";
    return 1;
  }
  Flow flow = flow_or.move_value();
  if (!flow.add_standard_scaler().ok()) {
    std::cerr << "add_standard_scaler failed\n";
    return 1;
  }
  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;
```

**Expected output**

```text
saved flow bundle to: <path>
flow bundle roundtrip exact prediction parity: true
nodej_flow_bundle_demo complete
```

### Example 3 (Demo Pattern)

```cpp
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
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
```

**Expected output**

```text
NodeJ Flow demo
classification flow metrics [accuracy, logloss]: 1 0.000228657
regression flow metrics [mse, r2]: 4.46648e-17 1
NodeJ Flow demo completed.
```
