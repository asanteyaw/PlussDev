# `FeatureTransformer::FeatureTransformer`

## API Signature

- `FeatureTransformer(remainder: FeatureRemainderPolicy = FeatureRemainderPolicy::Drop) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `remainder` (`FeatureRemainderPolicy`), default: `FeatureRemainderPolicy::Drop`
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

Option semantics used by this method:

- `FeatureRemainderPolicy` (`enum`):
  - `Drop`: Accepted enum choice for this method.
  - `PassThrough`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = FeatureTransformer(/* remainder */);
out.info("FeatureTransformer", value);
```

**Expected output**

```text
FeatureTransformer returned result
```
