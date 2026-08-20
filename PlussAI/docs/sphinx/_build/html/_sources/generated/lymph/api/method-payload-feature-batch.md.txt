# `Payload::feature_batch`

## API Signature

- `Payload::feature_batch() -> FeatureBatch`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `FeatureBatch`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::feature_batch();
out.info("feature_batch", value);
```

**Expected output**

```text
feature_batch returned FeatureBatch
```
