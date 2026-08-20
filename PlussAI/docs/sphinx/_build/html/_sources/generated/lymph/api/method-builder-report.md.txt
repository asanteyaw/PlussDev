# `Builder::report`

## API Signature

- `Builder::report(n: std::int64_t) -> pluss::table::DataFrame`
- `Builder::report(n: std::int64_t = -1) -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

- `n` (`std::int64_t`), default: `-1`
  Method argument; see signature type for accepted form.

## Output

- Returns `pluss::table::DataFrame`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::report(/* n */);
out.info("report", value);
```

**Expected output**

```text
report returned pluss::table::DataFrame
```

### Example 2 (Demo Pattern)

```cpp
        train_y,
        cv_options,
        train_options,
        assess_options,
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize);

    std::cout << "CV objective metric: rmse\n";
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
