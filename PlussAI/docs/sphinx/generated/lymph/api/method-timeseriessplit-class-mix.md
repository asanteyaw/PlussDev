# `TimeSeriesSplit::class_mix`

## API Signature

- `TimeSeriesSplit::class_mix(labels: const Series&) -> ClassMix`

## Notes

Summarizes class labels, class counts, class proportions, inverse-frequency weights, and imbalance ratio.

## Inputs

- `labels` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `ClassMix`.

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
auto value = TimeSeriesSplit::class_mix(/* labels */);
out.info("class_mix", value);
```

**Expected output**

```text
class_mix returned ClassMix
```
