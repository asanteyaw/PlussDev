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
libc++abi: terminating due to uncaught exception of type pluss::Error: DataReader::with_sizes: explicit sizes are only supported for host-buffer inputs
Triggered at: <path> (DataReader &pluss::detail::DataReader::with_sizes(std::span<const std::int64_t>))
Native stack trace:
  0  pluss::detail::DataReader::with_sizes(std::__1::span<long long const, 18446744073709551615ul>)(data_reader.h:250)
  1  pluss::codem::Series pluss::detail::DataReader::to_series<pluss::codem::Config const&>(pluss::detail::DataReader::Option, pluss::codem::Config const&) const(data_reader.h:295)
  2  pluss::codem::Series pluss::codem::value<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_value.h:13)
  3  pluss::codem::Series pluss::make_series<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory.h:960)
  4  main(nodej_flow_bundle_demo.cpp:13)
  5  start + 6992 [dyld]


[exit_code] -6
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
classification flow metrics [accuracy, logloss]: 1 1.44251e-05
regression flow metrics [mse, r2]: 4.46559e-17 1
NodeJ Flow demo completed.
```
