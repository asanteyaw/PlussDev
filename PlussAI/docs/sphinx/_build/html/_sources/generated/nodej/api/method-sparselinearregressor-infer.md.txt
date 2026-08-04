# `SparseLinearRegressor::infer`

## API Signature

- `SparseLinearRegressor::infer(x: const SparseCsrMatrix&) -> StatusOr<Series>`
- `SparseLinearRegressor::infer(x: const SparseTripletMatrix&) -> StatusOr<Series>`

## Notes

Computes predictions from prepared inputs using the currently learned state.

## Inputs

- `x` (`const SparseCsrMatrix& | const SparseTripletMatrix&`)
  Input feature data used by this call.

## Output

- Returns `StatusOr<Series>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `SparseCsrMatrix` (`struct`):
  - `std::vector<std::int64_t> row_ptr{}`
    Option field used by this method.
  - `std::vector<std::int64_t> col_ind{}`
    Option field used by this method.
  - `std::vector<double> values{}`
    Option field used by this method.
  - `std::int64_t rows = 0`
    Option field used by this method.
  - `std::int64_t cols = 0`
    Option field used by this method.

- `SparseTripletMatrix` (`struct`):
  - `Series row_indices = Series::undefined()`
    Option field used by this method.
  - `Series column_indices = Series::undefined()`
    Controls fold/trial/count sizing.
  - `Series values = Series::undefined()`
    Option field used by this method.
  - `std::int64_t rows = 0`
    Option field used by this method.
  - `std::int64_t cols = 0`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = SparseLinearRegressor::infer(/* x */);
if (!res.ok()) {
  out.error("infer failed", res.status().message);
} else {
  out.info("infer ok");
}
```

**Expected output**

```text
infer: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
  train.tolerance = 1e-8;
  train.random.seed = 77;
  train.random.deterministic = true;
  if (!flow.learn(batch, train).ok()) {
    std::cerr << "flow learn failed\n";
    return 1;
  }

  auto pred_before_or = flow.infer(batch.features);
  if (!pred_before_or.ok()) {
    std::cerr << "flow infer before save failed: " << pred_before_or.status().message << "\n";
    return 1;
  }
  const std::string model_dir = "<repo_root>/PlussAI/examples/NodeJ/trained_models";
  std::error_code mkdir_ec;
  std::filesystem::create_directories(model_dir, mkdir_ec);
  if (mkdir_ec) {
    std::cerr << "failed to create model directory: " << model_dir << " (" << mkdir_ec.message() << ")\n";
    return 1;
  }
  const std::string bundle_path = model_dir + "/nodej_flow_bundle_demo.bin";
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
  auto model_or = make_regressor(spec);
  if (!require_ok(model_or, name + " make_regressor")) return;
  std::unique_ptr<RegressorBase> model = model_or.move_value();
  if (!require_status(model->learn(train), name + " learn")) return;
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
