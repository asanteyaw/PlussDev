# `LabeledBatch`

## API Signature

- `LabeledBatch`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `LabeledBatch` object used by NodeJ/Lymph workflows.

## Notes

Container for supervised batches: features (`x`) plus aligned labels (`y`) and optional `sample_weights`. Use this as the primary input to training, validation, and tuning workflows.

## Fields

Public fields on this type:

- `FeatureBatch features{}`
- `Series y = Series::undefined()`
- `Series sample_weights = Series::undefined()`

## Examples

### Example 1

```cpp
void print_series(const std::string& label, const Series& x) {
  const auto v = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  std::cout << label << ":";
  for (double value : v) std::cout << " " << value;
  std::cout << "\n";
}

LabeledBatch make_xor_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -1.0, -1.0,
                                          -1.0,  1.0,
                                           1.0, -1.0,
                                           1.0,  1.0,
                                          -0.9, -0.9,
                                          -0.9,  0.9,
                                           0.9, -0.9,
                                           0.9,  0.9,
                                          -1.2, -1.1,
                                          -1.1,  1.2,
                                           1.1, -1.2,
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

### Example 2

```cpp
                                          -1.0, -0.5,
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

### Example 3

```cpp
                                            -1.0, -0.6,
                                            1.0, 0.6,
                                            2.0, 0.8,
                                            3.0, 1.0,
                                            std::numeric_limits<double>::quiet_NaN(), 0.7},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    ModelSpec cls_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
```

**Expected output**

```text
NodeJ Flow demo
classification flow metrics [accuracy, logloss]: 1 1.44251e-05
regression flow metrics [mse, r2]: 4.46559e-17 1
NodeJ Flow demo completed.
```
