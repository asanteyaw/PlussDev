# `classification_summary`

## API Signature

- `classification_summary(truth_labels: const Series&, predicted_labels: const Series&, label_order: RangeView::Of<double>, zero_division: double = 0.0) -> ClassReport`
- `classification_summary(truth_labels: const Series&, predicted_labels: const Series&, label_order: std::initializer_list<double>, zero_division: double = 0.0) -> ClassReport`

## Notes

Namespace-level API function.

## Inputs

- `label_order` (`RangeView::Of<double> | std::initializer_list<double>`)
  Target values aligned to input observations.
- `predicted_labels` (`const Series&`)
  Target values aligned to input observations.
- `truth_labels` (`const Series&`)
  Target values aligned to input observations.
- `zero_division` (`double`), default: `0.0`
  Method argument; see signature type for accepted form.

## Output

- Returns `ClassReport`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = classification_summary(/* truth_labels */, /* predicted_labels */, /* label_order */, /* zero_division */);
out.info("classification_summary", value);
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```
