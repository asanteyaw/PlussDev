# `confusion_table`

## API Signature

- `confusion_table(truth_labels: const Series&, predicted_labels: const Series&, label_order: std::initializer_list<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> StatusOr<ConfusionTable>`
- `confusion_table(truth_labels: const Series&, predicted_labels: const Series&, label_order: RangeView::Of<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> StatusOr<ConfusionTable>`

## Notes

Namespace-level API function.

## Inputs

- `label_order` (`RangeView::Of<double> | std::initializer_list<double>`)
  Target values aligned to input observations.
- `normalize` (`ConfusionNormalize`), default: `ConfusionNormalize::None`
  Method argument; see signature type for accepted form.
- `predicted_labels` (`const Series&`)
  Target values aligned to input observations.
- `truth_labels` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `StatusOr<ConfusionTable>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `ConfusionNormalize` (`enum`):
  - `None`: Disables this behavior.
  - `ByTruth`: Accepted enum choice for this method.
  - `ByPrediction`: Accepted enum choice for this method.
  - `Global`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = confusion_table(/* truth_labels */, /* predicted_labels */, /* label_order */, /* normalize */);
if (!res.ok()) {
  out.error("confusion_table failed", res.status().message);
} else {
  out.info("confusion_table ok");
}
```

**Expected output**

```text
confusion_table: ok
status.ok() == true
result.value() is available
```
