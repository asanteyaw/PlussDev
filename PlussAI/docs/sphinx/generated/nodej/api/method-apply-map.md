# `apply_map`

## API Signature

- `apply_map(batch: const FeatureBatch&, options: const InferOptions&) -> StatusOr<FeatureBatch>`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<FeatureBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `FeatureBatch` (`struct`):
  - `Series x = Series::undefined()`
    Option field used by this method.
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.

- `InferOptions` (`struct`):
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.
  - `MissingPolicy missing = MissingPolicy::Reject`
    Option field used by this method.
  - `UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error`
    Controls fold/trial/count sizing.
  - `double positive_threshold = 0.5`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = apply_map(/* batch */, /* options */);
if (!res.ok()) {
  out.error("apply_map failed", res.status().message);
} else {
  out.info("apply_map ok");
}
```

**Expected output**

```text
apply_map: ok
status.ok() == true
result.value() is available
```
