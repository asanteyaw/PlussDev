# `workflow::policy`

## API Signature

- `workflow::policy() -> const Policy&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Policy&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::policy();
out.info("policy", value);
```

**Expected output**

```text
policy returned const Policy&
```

### Example 2 (Demo Pattern)

```cpp
        {"iter_imputer_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(iter_imp_summary.feature_names.size())}, {1}))},
        {"standard_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(z_summary.feature_names.size())}, {1}))},
        {"robust_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(robust_summary.feature_names.size())}, {1}))},
        {"maxabs_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(maxabs_summary.feature_names.size())}, {1}))},
    });
    prep_state_frame.save_csv((out_dir / "prep_state_summary.csv").string());

    //========== modelling setup ========
    // This section defines common optimizer/fit controls and CV policy.
    TrainOptions train_options{};
    train_options.max_iterations = 3000;
    train_options.tolerance = 1e-8;
    train_options.random.seed = 7;
    train_options.random.deterministic = true;

    AssessOptions assess_options{};
    assess_options.metrics = {MetricKind::Rmse, MetricKind::R2, MetricKind::Mae};

    lymph::CrossValidationOptions cv_options{};
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
```

**Expected output**

```text
policy returned const Policy&
```
