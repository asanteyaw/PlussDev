# `sqrt`

## API Signature

- `sqrt() -> TargetTransform`

## Notes

Namespace-level API function.

## Inputs

None.

## Output

- Returns `TargetTransform`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = sqrt();
out.info("sqrt", value);
```

**Expected output**

```text
sqrt returned TargetTransform
```

### Example 2 (Demo Pattern)

```cpp
  for (double v : squared_errors) mean_sq += v;
  mean_sq /= static_cast<double>(std::max<std::size_t>(n, 1));
  double var_sq = 0.0;
  for (double v : squared_errors) {
    const double d = v - mean_sq;
    var_sq += d * d;
  }
  var_sq /= static_cast<double>(std::max<std::size_t>(n > 1 ? (n - 1) : 1, 1));
  const double se_sq = std::sqrt(var_sq / static_cast<double>(std::max<std::size_t>(n, 1)));
  const double z95 = 1.96;
  const double rmse_mean = std::sqrt(std::max(0.0, mean_sq));
  const double rmse_lo = std::sqrt(std::max(0.0, mean_sq - z95 * se_sq));
  const double rmse_hi = std::sqrt(std::max(0.0, mean_sq + z95 * se_sq));
  out.writeln("approx 95% rmse interval", std::vector<double>{rmse_lo, rmse_hi}, "rmse_point", rmse_mean);
  //========== End Cell 14 ========

  //========== Cell 15: Save, Load, Re-Infer ========
  const fs::path final_model_path = model_dir / "lymph_housing_ch2_best.bin";
  final_workflow.save(final_model_path.string(), train_prepared, train_y, train_options);
  auto restored = lymph::Model::load(final_model_path.string());
  const Series restored_pred = restored.infer(test_prepared);
```

**Expected output**

```text
sqrt returned TargetTransform
```
