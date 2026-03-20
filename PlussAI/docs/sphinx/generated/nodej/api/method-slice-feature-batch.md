# `slice_feature_batch`

## API Signature

- `slice_feature_batch(in: const FeatureBatch&, indices: const Series&) -> StatusOr<FeatureBatch>`

## Notes

Returns a row-subset of a feature-only batch using the provided index selection.

## Inputs

- `in` (`const FeatureBatch&`)
  Method argument; see signature type for accepted form.
- `indices` (`const Series&`)
  Pluss numeric vector/tensor input.

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
auto res = slice_feature_batch(/* in */, /* indices */);
if (!res.ok()) {
  out.error("slice_feature_batch failed", res.status().message);
} else {
  out.info("slice_feature_batch ok");
}
```

**Expected output**

```text
slice_feature_batch: ok
status.ok() == true
result.value() is available
```
