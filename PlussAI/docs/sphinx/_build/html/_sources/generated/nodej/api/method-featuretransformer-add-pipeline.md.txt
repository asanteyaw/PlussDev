# `FeatureTransformer::add_pipeline`

## API Signature

- `FeatureTransformer::add_pipeline(transformers: std::vector<std::unique_ptr<TransformerBase>>, feature_route: RangeView::Int) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `feature_route` (`RangeView::Int`)
  Input feature data used by this call.
- `transformers` (`std::vector<std::unique_ptr<TransformerBase>>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = FeatureTransformer::add_pipeline(/* transformers */, /* feature_route */);
if (!st.ok()) {
  out.error("add_pipeline failed", st.message);
} else {
  out.info("add_pipeline ok");
}
```

**Expected output**

```text
add_pipeline: ok
status.ok() == true
```
