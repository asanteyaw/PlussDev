# `LabeledBatch::has_labels`

## API Signature

- `LabeledBatch::has_labels() -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

None.

## Output

- Returns `bool`.

## Options

Option semantics used by this method:

- `LabeledBatch` (`struct`):
  - `FeatureBatch features{}`
    Option field used by this method.
  - `Series y = Series::undefined()`
    Option field used by this method.
  - `Series sample_weights = Series::undefined()`
    Optional column used as per-row training weight.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = LabeledBatch::has_labels();
out.info("has_labels", value);
```

**Expected output**

```text
has_labels: true
```
