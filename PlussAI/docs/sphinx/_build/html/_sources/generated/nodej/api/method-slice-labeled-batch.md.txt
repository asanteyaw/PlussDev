# `slice_labeled_batch`

## API Signature

- `slice_labeled_batch(in: const LabeledBatch&, indices: const Series&) -> StatusOr<LabeledBatch>`

## Notes

Returns a row-subset of a labeled batch using the provided index selection.

## Inputs

- `in` (`const LabeledBatch&`)
  Method argument; see signature type for accepted form.
- `indices` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `StatusOr<LabeledBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

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
auto res = slice_labeled_batch(/* in */, /* indices */);
if (!res.ok()) {
  out.error("slice_labeled_batch failed", res.status().message);
} else {
  out.info("slice_labeled_batch ok");
}
```

**Expected output**

```text
slice_labeled_batch: ok
status.ok() == true
result.value() is available
```
