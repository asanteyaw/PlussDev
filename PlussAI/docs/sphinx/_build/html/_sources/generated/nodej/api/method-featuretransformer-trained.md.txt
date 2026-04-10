# `FeatureTransformer::trained`

## API Signature

- `FeatureTransformer::trained() -> bool`

## Notes

Boolean state check for object readiness.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = FeatureTransformer::trained();
out.info("trained", value);
```

**Expected output**

```text
trained: true
```
