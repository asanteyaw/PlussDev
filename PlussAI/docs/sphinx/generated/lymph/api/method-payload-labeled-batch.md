# `Payload::labeled_batch`

## API Signature

- `Payload::labeled_batch() -> LabeledBatch`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `LabeledBatch`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::labeled_batch();
out.info("labeled_batch", value);
```

**Expected output**

```text
labeled_batch returned LabeledBatch
```
