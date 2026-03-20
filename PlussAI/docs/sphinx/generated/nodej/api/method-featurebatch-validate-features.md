# `FeatureBatch::validate_features`

## API Signature

- `FeatureBatch::validate_features(batch: const FeatureBatch&) -> Status`

## Notes

Validates shape/layout/typing invariants for this data container before downstream use.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.

## Output

- Returns a status object indicating success/failure and an error message on failure.

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
auto st = FeatureBatch::validate_features(/* batch */);
if (!st.ok()) {
  out.error("validate_features failed", st.message);
} else {
  out.info("validate_features ok");
}
```

**Expected output**

```text
validate_features: ok
status.ok() == true
```
