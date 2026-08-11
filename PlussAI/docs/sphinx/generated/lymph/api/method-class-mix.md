# `class_mix`

## API Signature

- `class_mix(frame: const pluss::table::DataFrame&, label_feature: std::string_view) -> ClassMix`

## Notes

Summarizes class labels, class counts, class proportions, inverse-frequency weights, and imbalance ratio.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `label_feature` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns `ClassMix`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = class_mix(/* frame */, /* label_feature */);
out.info("class_mix", value);
```

**Expected output**

```text
class_mix returned ClassMix
```
