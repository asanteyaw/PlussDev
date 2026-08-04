# `class_mix`

## API Signature

- `class_mix(labels: const Series&) -> ClassMix`

## Notes

Summarizes class labels, class counts, class proportions, inverse-frequency weights, and imbalance ratio.

## Inputs

- `labels` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `ClassMix`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = class_mix(/* labels */);
out.info("class_mix", value);
```

**Expected output**

```text
class_mix returned ClassMix
```
