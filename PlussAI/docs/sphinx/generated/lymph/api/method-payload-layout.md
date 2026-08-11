# `Payload::layout`

## API Signature

- `Payload::layout(layout: ObservationLayout) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `layout` (`ObservationLayout`)
  Target values aligned to input observations.

## Output

- Returns `Payload&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::layout(/* layout */);
out.info("layout", value);
```

**Expected output**

```text
layout returned Payload&
```

### Example 2 (Demo Pattern)

```cpp
  const Series y = pluss::make_series(std::vector<double>{
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0},
                                      {16},
                                      f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

ModelSpec linear_logistic_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
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
                                           1.0,  0.5,
                                           2.0,  1.0,
                                          -1.5, -0.6,
                                           1.5,  0.7},
                                      {6, 2},
                                      cfg);
  const Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0, 0.0, 1.0}, {6}, cfg);
  const LabeledBatch batch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };

  ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
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
