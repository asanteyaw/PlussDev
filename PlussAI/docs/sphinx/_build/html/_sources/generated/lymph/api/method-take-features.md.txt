# `take_features`

## API Signature

- `take_features(batch: const FeatureBatch&, feature_indices: RangeView::Int) -> FeatureBatch`
- `take_features(batch: const FeatureBatch&, feature_indices: std::initializer_list<std::int64_t>) -> FeatureBatch`

## Notes

Namespace-level API function.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `feature_indices` (`RangeView::Int | std::initializer_list<std::int64_t>`)
  Input feature data used by this call.

## Output

- Returns `FeatureBatch`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = take_features(/* batch */, /* feature_indices */);
out.info("take_features", value);
```

**Expected output**

```text
take_features returned FeatureBatch
```
