# `Flow::add_feature_transformer`

## API Signature

- `Flow::add_feature_transformer(transformer: prep::FeatureTransformer) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`prep::FeatureTransformer`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::add_feature_transformer(/* transformer */);
if (!st.ok()) {
  out.error("add_feature_transformer failed", st.message);
} else {
  out.info("add_feature_transformer ok");
}
```

**Expected output**

```text
add_feature_transformer: ok
status.ok() == true
```
