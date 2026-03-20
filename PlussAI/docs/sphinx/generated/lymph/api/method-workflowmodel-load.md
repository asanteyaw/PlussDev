# `WorkflowModel::load`

## API Signature

- `WorkflowModel::load(path: const std::string&) -> WorkflowModel`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `path` (`const std::string&`)
  Path to persisted model/data artifact.

## Output

- Returns `WorkflowModel`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::load(/* path */);
out.info("load", value);
```

**Expected output**

```text
load returned WorkflowModel
```

### Example 2 (Demo Pattern)

```cpp
            lymph::workflow::stage::standard_scaler(),
        },
    };
    linear_workflow.learn_(train_x, train_y, train_options);
    const auto linear_metrics = linear_workflow.assess(test_x, test_y, assess_options);
    std::cout << "linear test metrics [rmse, r2, mae]: " << linear_metrics.rmse << " " << linear_metrics.r2 << " "
              << linear_metrics.mae << "\n";
    std::cout << "linear params: intercept=" << linear_workflow.intercept() << " coeff=" << linear_workflow.coeff() << "\n";

    const Series linear_pred_series = linear_workflow.infer(test_x);
    const fs::path linear_model_path = model_dir / "bike_sharing_linear.bin";
    linear_workflow.save(linear_model_path.string(), train_x, train_y, train_options);
    const auto linear_restored = lymph::Workflow::load(linear_model_path.string());
    const Series linear_pred_restored_series = linear_restored.infer(test_x);
    const Series y_true = test_y.to(pluss::pCPU, ScalarType::Float64).reshape({test_y.numel()});

    const Series linear_pred = linear_pred_series.to(pluss::pCPU, ScalarType::Float64).reshape({linear_pred_series.numel()});
    const double linear_max_abs_diff = lymph::max_abs_diff(linear_pred_series, linear_pred_restored_series);
    std::cout << "linear restored prediction max_abs_diff: " << linear_max_abs_diff << "\n";

    DataFrame linear_frame({
```

**Expected output**

```text
load returned WorkflowModel
```
