# `FeatureTransformer::remainder_policy`

## API Signature

- `FeatureTransformer::remainder_policy() -> FeatureRemainderPolicy`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `FeatureRemainderPolicy`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = FeatureTransformer::remainder_policy();
out.info("remainder_policy", value);
```

**Expected output**

```text
remainder_policy returned FeatureRemainderPolicy
```
