# `Classification::prediction`

## API Signature

- `Classification::prediction() -> const Series&`

## Notes

Computes predictions from prepared inputs using the currently learned state.

## Inputs

None.

## Output

- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Classification::prediction();
out.info("prediction", value);
```

**Expected output**

```text
prediction returned const Series&
```

### Example 2 (Demo Pattern)

```cpp
  }

  const auto before = pred_before_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  const auto after = pred_after_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  bool same = before.size() == after.size();
  for (std::size_t i = 0; same && i < before.size(); ++i) {
    same = std::abs(before[i] - after[i]) < 1e-9;
  }
  std::cout << "flow bundle roundtrip exact prediction parity: " << (same ? "true" : "false") << "\n";
  std::cout << "nodej_flow_bundle_demo complete\n";
  return same ? 0 : 2;
}
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

### Example 3 (Demo Pattern)

```cpp
    std::cout << "test metrics [rmse, r2, mae]: " << test_metrics.rmse << " " << test_metrics.r2 << " " << test_metrics.mae << "\n";

    const Series pred = best.infer(test_x);
    const fs::path best_model_path = model_dir / "hour_best_workflow.bin";
    best.save(best_model_path.string(), train_x, train_y, train_options);
    auto restored = nja::Model::load(best_model_path.string());
    const Series pred_restored = restored.infer(test_x);
    const double restored_max_abs_diff = nja::max_abs_diff(pred, pred_restored);
    std::cout << "restored prediction max_abs_diff: " << restored_max_abs_diff << "\n";

    const Series y_true = test_y.to(pluss::pCPU, ScalarType::Float64).reshape({test_y.numel()});
    const Series y_pred = pred.to(pluss::pCPU, ScalarType::Float64).reshape({pred.numel()});
    DataFrame pred_frame({
        {"y_true", Thread(y_true)},
        {"y_pred", Thread(y_pred)},
        {"residual", Thread(y_true.sub(y_pred))},
    });
    pred_frame.save_csv((out_dir / "hour_workflow_tuning_predictions.csv").string());
    pred_frame.plot()
        .scatter("y_true", "y_pred")
        .headline("Observed vs predicted bike demand (best workflow)")
```

**Expected output**

```text
CV objective metric: rmse
candidate=linear_ridge objective=102.872 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[102.87158] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.41650024] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[77.611594]
candidate=decision_tree objective=44.9727 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[44.972693] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.88843998] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[27.61723]
candidate=random_forest objective=81.1235 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[81.123538] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.63710967] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
...
```
