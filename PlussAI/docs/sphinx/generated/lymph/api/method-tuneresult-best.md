# `TuneResult::best`

## API Signature

- `TuneResult::best() -> const Workflow&`
- `TuneResult::best() -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Workflow&`.
- Returns `const Workflow&`.

## Options

Option semantics used by this method:

- `TuneResult` (`struct`):
  - `TuneReport report{}`
    Option field used by this method.
  - `Workflow best_workflow{}`
    Option field used by this method.
  - `std::vector<Workflow> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TuneResult::best();
out.info("best", value);
```

**Expected output**

```text
best returned const Workflow&
```

### Example 2 (Demo Pattern)

```cpp
                        TuneObjectivePolicy::Auto);
  if (!require_ok(sweep_or, "sweep")) return 1;

  std::cout << "sweep trial objectives:";
  for (const auto& trial : sweep_or.value().trials) {
    std::cout << " " << trial.objective;
  }
  std::cout << "\n";
  std::cout << "sweep best index: " << sweep_or.value().best_index << "\n";

  auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
    (void)seed;
    return StatusOr<ModelSpec>((trial_index % 2) == 0 ? linear_logistic_spec() : rbf_kernel_spec());
  };

  auto probe_or = probe(sampler,
                        4,
                        batch,
                        split,
                        train,
                        tune_assess,
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
    std::cout << "CV objective metric: rmse\n";
    for (const auto& trial : tuned.report.trials) {
      std::cout << "candidate=" << trial.name << " objective=" << trial.objective
                << " cv_rmse=" << trial.cv.mean.rmse
                << " cv_r2=" << trial.cv.mean.r2
                << " cv_mae=" << trial.cv.mean.mae << "\n";
    }
    std::cout << "best candidate: " << tuned.report.best_name << "\n";

    auto best = std::move(tuned.best_workflow);
    const auto test_metrics = best.assess(test_x, test_y, assess_options);
    std::cout << "test metrics [rmse, r2, mae]: " << test_metrics.rmse << " " << test_metrics.r2 << " " << test_metrics.mae << "\n";

    const Series pred = best.infer(test_x);
    const fs::path best_model_path = model_dir / "hour_best_workflow.bin";
    best.save(best_model_path.string(), train_x, train_y, train_options);
    auto restored = nja::Workflow::load(best_model_path.string());
    const Series pred_restored = restored.infer(test_x);
    const double restored_max_abs_diff = nja::max_abs_diff(pred, pred_restored);
    std::cout << "restored prediction max_abs_diff: " << restored_max_abs_diff << "\n";
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
