# `take_features`

## API Signature

- `take_features(batch: const FeatureBatch&, feature_indices: std::initializer_list<std::int64_t>) -> StatusOr<FeatureBatch>`
- `take_features(batch: const FeatureBatch&, feature_indices: RangeView::Int) -> StatusOr<FeatureBatch>`

## Notes

Namespace-level API function.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `feature_indices` (`RangeView::Int | std::initializer_list<std::int64_t>`)
  Input feature data used by this call.

## Output

- Returns `StatusOr<FeatureBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `FeatureBatch` (`struct`):
  - `Series x = Series::undefined()`
    Option field used by this method.
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = take_features(/* batch */, /* feature_indices */);
if (!res.ok()) {
  out.error("take_features failed", res.status().message);
} else {
  out.info("take_features ok");
}
```

**Expected output**

```text
take_features: ok
status.ok() == true
result.value() is available
```
