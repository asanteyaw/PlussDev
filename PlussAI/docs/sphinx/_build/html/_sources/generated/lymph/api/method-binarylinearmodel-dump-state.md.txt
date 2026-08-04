# `BinaryLinearModel::dump_state`

## API Signature

- `BinaryLinearModel::dump_state() -> std::vector<Series>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<Series>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = BinaryLinearModel::dump_state();
out.info("dump_state", value);
```

**Expected output**

```text
dump_state returned std::vector<Series>
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
dump_state returned std::vector<Series>
```
