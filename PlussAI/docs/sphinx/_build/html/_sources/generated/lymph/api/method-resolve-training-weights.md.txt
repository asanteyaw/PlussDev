# `resolve_training_weights`

## API Signature

- `resolve_training_weights(labels: const Series&, batch_sample_weights: const Series& = Series::undefined() -> Series`

## Notes

Returns normalized per-sample training weights from labels, optional sample weights, and class-weight options.

## Inputs

- `batch_sample_weights` (`const Series&`), default: `Series::undefined(`
  Optional per-row weights aligned to input observations.
- `labels` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = resolve_training_weights(/* labels */, /* batch_sample_weights */);
out.info("resolve_training_weights", value);
```

**Expected output**

```text
resolve_training_weights returned Series
```
