# `variance_prune`

## API Signature

- `variance_prune(batch: const FeatureBatch&, variance_floor: double = 1e-12) -> StatusOr<VariancePruneResult>`

## Notes

Namespace-level API function.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `variance_floor` (`double`), default: `1e-12`
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<VariancePruneResult>`. Check `.ok()` first; on success read `.value()` for the computed result.

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
auto res = variance_prune(/* batch */, /* variance_floor */);
if (!res.ok()) {
  out.error("variance_prune failed", res.status().message);
} else {
  out.info("variance_prune ok");
}
```

**Expected output**

```text
variance_prune: ok
status.ok() == true
result.value() is available
```
