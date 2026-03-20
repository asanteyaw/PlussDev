# `max_abs_diff`

## API Signature

- `max_abs_diff(lhs: const Series&, rhs: const Series&) -> double`

## Notes

Namespace-level API function.

## Inputs

- `lhs` (`const Series&`)
  Pluss numeric vector/tensor input.
- `rhs` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = max_abs_diff(/* lhs */, /* rhs */);
out.info("max_abs_diff", value);
```

**Expected output**

```text
max_abs_diff: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
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
        {"y_true", Thread(y_true)},
        {"y_pred", Thread(linear_pred)},
        {"residual", Thread(y_true.sub(linear_pred))},
    });
    linear_frame.save_csv((out_dir / "bike_sharing_test_predictions_linear.csv").string());
```

**Expected output**

```text
max_abs_diff: <numeric_result>
```
