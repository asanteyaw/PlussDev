# `WorkflowModel::workflow`

## API Signature

- `WorkflowModel::workflow() -> const Workflow&`
- `WorkflowModel::workflow() -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Workflow&`.
- Returns `const Workflow&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::workflow();
out.info("workflow", value);
```

**Expected output**

```text
workflow returned const Workflow&
```

### Example 2 (Demo Pattern)

```cpp
    auto linear_workflow = lymph::Workflow{
        .model = lymph::linear_factory::linear_regression(LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::None,
            .regularization_strength = 0.0,
            .fit_intercept = true,
        }),
        .stages = {
            lymph::workflow::stage::simple_imputer(
                lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median}),
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
```

**Expected output**

```text
workflow returned const Workflow&
```
