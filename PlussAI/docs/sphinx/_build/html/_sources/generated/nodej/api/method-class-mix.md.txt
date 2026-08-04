# `class_mix`

## API Signature

- `class_mix(labels: const Series&) -> StatusOr<ClassMix>`

## Notes

Summarizes class labels, class counts, class proportions, inverse-frequency weights, and imbalance ratio.

## Inputs

- `labels` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `StatusOr<ClassMix>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = class_mix(/* labels */);
if (!res.ok()) {
  out.error("class_mix failed", res.status().message);
} else {
  out.info("class_mix ok");
}
```

**Expected output**

```text
class_mix: ok
status.ok() == true
result.value() is available
```
