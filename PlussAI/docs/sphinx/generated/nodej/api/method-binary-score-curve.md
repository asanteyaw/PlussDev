# `binary_score_curve`

## API Signature

- `binary_score_curve(scores: const Series&, labels01: const Series&) -> StatusOr<BinaryScoreCurve>`

## Notes

Namespace-level API function.

## Inputs

- `labels01` (`const Series&`)
  Target values aligned to input observations.
- `scores` (`const Series&`)
  Metric selection or score-control argument for evaluation/tuning.

## Output

- Returns `StatusOr<BinaryScoreCurve>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = binary_score_curve(/* scores */, /* labels01 */);
if (!res.ok()) {
  out.error("binary_score_curve failed", res.status().message);
} else {
  out.info("binary_score_curve ok");
}
```

**Expected output**

```text
binary_score_curve: ok
status.ok() == true
result.value() is available
```
