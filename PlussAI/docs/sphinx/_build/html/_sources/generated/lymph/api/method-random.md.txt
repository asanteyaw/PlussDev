# `random`

## API Signature

- `random(axes: std::vector<GridAxis>, trials: std::int64_t, cv: metrics::ValidationOptions) -> Search`
- `random(axes: std::vector<GridAxis>, trials: std::int64_t, random: RandomState, cv: metrics::ValidationOptions) -> Search`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`std::vector<GridAxis>`)
  Input feature data used by this call.
- `cv` (`metrics::ValidationOptions`)
  Method argument; see signature type for accepted form.
- `random` (`RandomState`)
  Randomization control for reproducible behavior.
- `trials` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns `Search`.

## Options

Option semantics used by this method:

- `ValidationOptions` (`struct`):
  - `CVPlanKind plan = CVPlanKind::CrossVal`
    Option field used by this method.
  - `double holdout_fraction = 0.2`
    Option field used by this method.
  - `std::int64_t folds = 5`
    Controls fold/trial/count sizing.
  - `std::int64_t repeats = 1`
    Option field used by this method.
  - `std::int64_t time_gap = 0`
    Option field used by this method.
  - `std::int64_t time_test_size = 0`
    Option field used by this method.
  - `std::int64_t time_max_train_size = 0`
    Controls fold/trial/count sizing.
  - `bool stratified = false`
    Option field used by this method.
  - `Series groups = Series::undefined()`
    Option field used by this method.
  - `bool reliability_defaults = true`
    Option field used by this method.
  - `bool shuffle = true`
    Option field used by this method.
  - `std::int64_t max_parallel = 0`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = random(/* axes */, /* trials */, /* cv */);
out.info("random", value);
```

**Expected output**

```text
random returned Search
```

### Example 2 (Demo Pattern)

```cpp
  std::cout << "NodeJ CV + tune demo\n";

  const LabeledBatch batch = make_xor_batch();

  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
  split.folds = 4;
  split.shuffle = true;
  split.random.seed = 73;
  split.random.deterministic = true;

  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;

  AssessOptions assess{};
  assess.metrics = {
      MetricKind::Accuracy,
      MetricKind::BalancedAccuracy,
      MetricKind::Precision,
      MetricKind::Recall,
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
  Flow flow = flow_or.move_value();
  if (!flow.add_standard_scaler().ok()) {
    std::cerr << "add_standard_scaler failed\n";
    return 1;
  }
  TrainOptions train{};
  train.max_iterations = 2500;
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
```

**Expected output**

```text
libc++abi: terminating due to uncaught exception of type pluss::Error: DataReader::with_sizes: explicit sizes are only supported for host-buffer inputs
Triggered at: <path> (DataReader &pluss::detail::DataReader::with_sizes(std::span<const std::int64_t>))
Native stack trace:
  0  pluss::detail::DataReader::with_sizes(std::__1::span<long long const, 18446744073709551615ul>)(data_reader.h:265)
  1  pluss::codem::Series pluss::detail::DataReader::to_series<pluss::codem::Config const&>(pluss::detail::DataReader::Option, pluss::codem::Config const&) const(data_reader.h:310)
  2  pluss::codem::Series pluss::codem::value<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_value.h:13)
  3  pluss::codem::Series pluss::series::make<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory.h:973)
  4  pluss::codem::Series pluss::make_series<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory_compat.h:20)
  5  main(nodej_flow_bundle_demo.cpp:13)
  6  start + 6992 [dyld]


...
```
