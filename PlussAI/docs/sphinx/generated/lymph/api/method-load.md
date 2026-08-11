# `load`

## API Signature

- `load(path: const std::string&) -> Workflow`
- `load(path: const std::string&) -> Workflow`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `path` (`const std::string&`)
  Path to persisted model/data artifact.

## Output

- Returns `Workflow`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = load(/* path */);
out.info("load", value);
```

**Expected output**

```text
load returned Workflow
```

### Example 2 (Demo Pattern)

```cpp
    auto best = std::move(tuned.best());
    const auto test_metrics = best.assess(test_x, test_y, assess_options);
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
