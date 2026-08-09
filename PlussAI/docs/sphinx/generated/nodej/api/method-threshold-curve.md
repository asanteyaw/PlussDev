# `threshold_curve`

## API Signature

- `threshold_curve(scores: const Series&, labels: const Series&) -> StatusOr<ThresholdCurve>`

## Notes

Namespace-level API function.

## Inputs

- `labels` (`const Series&`)
  Target values aligned to input observations.
- `scores` (`const Series&`)
  Metric selection or score-control argument for evaluation/tuning.

## Output

- Returns `StatusOr<ThresholdCurve>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = threshold_curve(/* scores */, /* labels */);
if (!res.ok()) {
  out.error("threshold_curve failed", res.status().message);
} else {
  out.info("threshold_curve ok");
}
```

**Expected output**

```text
threshold_curve: ok
status.ok() == true
result.value() is available
```
