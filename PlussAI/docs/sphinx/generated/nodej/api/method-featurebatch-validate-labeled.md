# `FeatureBatch::validate_labeled`

## API Signature

- `FeatureBatch::validate_labeled(batch: const LabeledBatch&) -> Status`

## Notes

Validates shape/layout/typing invariants for this data container before downstream use.

## Inputs

- `batch` (`const LabeledBatch&`)
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
auto st = FeatureBatch::validate_labeled(/* batch */);
if (!st.ok()) {
  out.error("validate_labeled failed", st.message);
} else {
  out.info("validate_labeled ok");
}
```

**Expected output**

```text
validate_labeled: ok
status.ok() == true
```

### Example 2 (Demo Pattern)

```cpp
  train_batch.y = train_y.to(train_standard_scaled.platform(), ScalarType::Float64).reshape({train_standard_scaled.sizes()[0]});
  train_batch.sample_weights = Series::undefined();

  LabeledBatch test_batch{};
  test_batch.features = FeatureBatch{.x = test_standard_scaled, .layout = ObservationLayout::Rows};
  test_batch.y = test_y.to(test_standard_scaled.platform(), ScalarType::Float64).reshape({test_standard_scaled.sizes()[0]});
  test_batch.sample_weights = Series::undefined();

  Status train_batch_check = validate_labeled(train_batch);
  if (!train_batch_check.ok()) {
    std::cerr << "validate_labeled(train_batch) failed: " << train_batch_check.message << "\n";
    return 1;
  }
  Status test_batch_check = validate_labeled(test_batch);
  if (!test_batch_check.ok()) {
    std::cerr << "validate_labeled(test_batch) failed: " << test_batch_check.message << "\n";
    return 1;
  }

  TrainOptions train_options{};
  train_options.max_iterations = 3000;
```

**Expected output**

```text
validate_labeled: ok
status.ok() == true
```
