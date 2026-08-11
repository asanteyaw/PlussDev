# `TimeSeriesSplit::confusion_table`

## API Signature

- `TimeSeriesSplit::confusion_table(truth_labels: const Series&, predicted_labels: const Series&, label_order: RangeView::Of<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> ConfusionTable`
- `TimeSeriesSplit::confusion_table(truth_labels: const Series&, predicted_labels: const Series&, label_order: std::initializer_list<double>, normalize: ConfusionNormalize = ConfusionNormalize::None) -> ConfusionTable`

## Notes

Instance method on `self`.

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

Option semantics used by this method:

- `TimeSeriesSplit` (`struct`):
  - `pluss::table::DataFrame train_set{}`
    Controls fold/trial/count sizing.
  - `pluss::table::DataFrame test_set{}`
    Option field used by this method.
  - `LabeledBatch train_batch{}`
    Controls fold/trial/count sizing.
  - `LabeledBatch test_batch{}`
    Option field used by this method.
  - `IndexSplit indices{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TimeSeriesSplit::confusion_table(/* truth_labels */, /* predicted_labels */, /* label_order */, /* normalize */);
out.info("confusion_table", value);
```

**Expected output**

```text
confusion_table returned ConfusionTable
```
