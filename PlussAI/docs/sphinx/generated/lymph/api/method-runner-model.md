# `Runner::model`

## API Signature

- `Runner::model() -> const Model&`
- `Runner::model() -> const Model&`
- `Runner::model() -> const Model&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Model&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Runner::model();
out.info("model", value);
```

**Expected output**

```text
model returned const Model&
```

### Example 2 (Demo Pattern)

```cpp
    linear_frame.plot()
        .scatter("y_true", "y_pred")
        .headline("Observed vs predicted bike demand (linear)")
        .save_pdf((out_dir / "bike_pred_vs_true_linear.pdf").string());
    linear_frame.plot()
        .hist("residual", 80)
        .headline("Residual distribution (linear)")
        .save_pdf((out_dir / "bike_residual_hist_linear.pdf").string());
    std::cout << "saved model (linear): " << linear_model_path << "\n";

    auto tree_workflow = lymph::models::decision_tree_regressor(TreeFamilyOptions{
        .criterion = TreeCriterion::Mse,
        .max_depth = 18,
        .min_samples_leaf = 8,
        .min_samples_split = 24,
    });
    tree_workflow.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
    tree_workflow.learn_(train_x, train_y, train_options);
    const auto tree_metrics = tree_workflow.assess(test_x, test_y, assess_options);
    std::cout << "tree test metrics [rmse, r2, mae]: " << tree_metrics.rmse << " " << tree_metrics.r2 << " "
              << tree_metrics.mae << "\n";
```

**Expected output**

```text
model returned const Model&
```
