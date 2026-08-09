# `compare_series`

## API Signature

- `compare_series(lhs: const Series&, rhs: const Series&) -> SeriesCompareReport`

## Notes

Namespace-level API function.

## Inputs

- `lhs` (`const Series&`)
  Pluss numeric vector/tensor input.
- `rhs` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `SeriesCompareReport`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = compare_series(/* lhs */, /* rhs */);
out.info("compare_series", value);
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```

### Example 2 (Demo Pattern)

```cpp
  out.writeln("approx 95% rmse interval", std::vector<double>{rmse_lo, rmse_hi}, "rmse_point", rmse_mean);
  //========== End Cell 14 ========

  //========== Cell 15: Save, Load, Re-Infer ========
  const fs::path final_model_path = model_dir / "lymph_housing_ch2_best.bin";
  final_workflow.save(final_model_path.string(), train_prepared, train_y, train_options);
  auto restored = lymph::Model::load(final_model_path.string());
  const Series restored_pred = restored.infer(test_prepared);
  const auto parity = lymph::compare_series(final_pred, restored_pred);
  out.writeln("restored parity", parity.max_abs_diff, parity.mae);

  DataFrame prediction_frame({
      {"y_true", test_y},
      {"y_pred", Thread(final_pred)},
      {"y_pred_restored", Thread(restored_pred)},
  });
  prediction_frame.save_csv((output_dir / "housing_ch2_test_predictions.csv").string());
  out.writeln("saved predictions", (output_dir / "housing_ch2_test_predictions.csv").string());
  out.writeln("saved model", final_model_path.string());
  //========== End Cell 15 ========
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```
