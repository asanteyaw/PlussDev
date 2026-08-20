# `CompareResult::best`

## API Signature

- `CompareResult::best() -> const Model&`
- `CompareResult::best() -> Model&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Model&`.
- Returns `const Model&`.

## Options

Option semantics used by this method:

- `CompareResult` (`struct`):
  - `CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::optional<Model> worst_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.
  - `std::vector<Model> bottom_fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CompareResult::best();
out.info("best", value);
```

**Expected output**

```text
best returned const Model&
```

### Example 2 (Demo Pattern)

```cpp
    for (const auto& trial : tuned.report.trials) {
      std::cout << "candidate=" << trial.name << " objective=" << trial.objective
                << " cv_rmse=" << trial.cv.mean.rmse
                << " cv_r2=" << trial.cv.mean.r2
                << " cv_mae=" << trial.cv.mean.mae << "\n";
    }
    std::cout << "best candidate: " << tuned.report.best_name << "\n";

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
