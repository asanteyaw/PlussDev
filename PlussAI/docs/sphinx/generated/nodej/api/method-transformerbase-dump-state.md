# `TransformerBase::dump_state`

## API Signature

- `TransformerBase::dump_state() -> StatusOr<std::vector<Series>>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StatusOr<std::vector<Series>>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = TransformerBase::dump_state();
if (!res.ok()) {
  out.error("dump_state failed", res.status().message);
} else {
  out.info("dump_state ok");
}
```

**Expected output**

```text
dump_state: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
  StatusOr<Series> test_maxabs_scaled_or = maxabs_scaler.map(test_simple_batch);
  if (!test_maxabs_scaled_or.ok()) {
    std::cerr << "MaxAbsScaler.map(test) failed: " << test_maxabs_scaled_or.status().message << "\n";
    return 1;
  }
  Series test_maxabs_scaled = test_maxabs_scaled_or.move_value();

  // This section saves compact prep-state diagnostics.
  StatusOr<std::vector<Series>> simple_state_or = simple_imputer.dump_state();
  if (!simple_state_or.ok()) {
    std::cerr << "SimpleImputer.dump_state failed: " << simple_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> simple_state = simple_state_or.move_value();

  StatusOr<std::vector<Series>> knn_state_or = knn_imputer.dump_state();
  if (!knn_state_or.ok()) {
    std::cerr << "NeighborImputer.dump_state failed: " << knn_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> knn_state = knn_state_or.move_value();
```

**Expected output**

```text
dump_state: ok
status.ok() == true
result.value() is available
```
