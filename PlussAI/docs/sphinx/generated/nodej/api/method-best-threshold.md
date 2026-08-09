# `best_threshold`

## API Signature

- `best_threshold(scores: const Series&, labels: const Series&, objective: MetricKind = MetricKind::F1) -> StatusOr<double>`

## Notes

Namespace-level API function.

## Inputs

- `labels` (`const Series&`)
  Target values aligned to input observations.
- `objective` (`MetricKind`), default: `MetricKind::F1`
  Metric selection or score-control argument for evaluation/tuning.
- `scores` (`const Series&`)
  Metric selection or score-control argument for evaluation/tuning.

## Output

- Returns `StatusOr<double>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `MetricKind` (`enum`):
  - `Accuracy`: Accepted enum choice for this method.
  - `BalancedAccuracy`: Accepted enum choice for this method.
  - `Precision`: Accepted enum choice for this method.
  - `Recall`: Accepted enum choice for this method.
  - `F1`: Accepted enum choice for this method.
  - `RocAuc`: Accepted enum choice for this method.
  - `LogLoss`: Accepted enum choice for this method.
  - `Mse`: Accepted enum choice for this method.
  - `Rmse`: Accepted enum choice for this method.
  - `Mae`: Accepted enum choice for this method.
  - `R2`: Accepted enum choice for this method.
  - `QuantileLoss`: Accepted enum choice for this method.
  - `Sensitivity`: Accepted enum choice for this method.
  - `Specificity`: Accepted enum choice for this method.
  - `GeometricMean`: Accepted enum choice for this method.
  - `BalanceQuality`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = best_threshold(/* scores */, /* labels */, /* objective */);
if (!res.ok()) {
  out.error("best_threshold failed", res.status().message);
} else {
  out.info("best_threshold ok");
}
```

**Expected output**

```text
best_threshold: ok
status.ok() == true
result.value() is available
```
