# `confusion_matrix`

## API Signature

- `confusion_matrix(truth_labels: const Series&, predicted_labels: const Series&, label_order: RangeView::Of<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> ConfusionTable`
- `confusion_matrix(truth_labels: const Series&, predicted_labels: const Series&, label_order: std::initializer_list<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> ConfusionTable`

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

- Returns `ConfusionTable`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = confusion_matrix(/* truth_labels */, /* predicted_labels */, /* label_order */, /* normalize */);
out.info("confusion_matrix", value);
```

**Expected output**

```text
confusion_matrix returned ConfusionTable
```
