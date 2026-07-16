# `classification_summary`

## API Signature

- `classification_summary(truth_labels: const Series&, predicted_labels: const Series&, label_order: std::initializer_list<double>, zero_division: double = 0.0) -> StatusOr<ClassReport>`
- `classification_summary(truth_labels: const Series&, predicted_labels: const Series&, label_order: RangeView::Of<double>, zero_division: double = 0.0) -> StatusOr<ClassReport>`

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

- Returns `StatusOr<ClassReport>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = classification_summary(/* truth_labels */, /* predicted_labels */, /* label_order */, /* zero_division */);
if (!res.ok()) {
  out.error("classification_summary failed", res.status().message);
} else {
  out.info("classification_summary ok");
}
```

**Expected output**

```text
classification_summary: ok
status.ok() == true
result.value() is available
```
