# `FeatureTransformer::add_stage`

## API Signature

- `FeatureTransformer::add_stage(transformer: std::unique_ptr<TransformerBase>, feature_route: RangeView::Int) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `feature_route` (`RangeView::Int`)
  Input feature data used by this call.
- `transformer` (`std::unique_ptr<TransformerBase>`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = FeatureTransformer::add_stage(/* transformer */, /* feature_route */);
if (!st.ok()) {
  out.error("add_stage failed", st.message);
} else {
  out.info("add_stage ok");
}
```

**Expected output**

```text
add_stage: ok
status.ok() == true
```
