# `OnlineLinearRegressor::core`

## API Signature

- `OnlineLinearRegressor::core() -> ::pluss::ai::nodej::online::OnlineLinearRegressor&`
- `OnlineLinearRegressor::core() -> const ::pluss::ai::nodej::online::OnlineLinearRegressor&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `::pluss::ai::nodej::online::OnlineLinearRegressor&`.
- Returns `const ::pluss::ai::nodej::online::OnlineLinearRegressor&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = OnlineLinearRegressor::core();
out.info("core", value);
```

**Expected output**

```text
core returned ::pluss::ai::nodej::online::OnlineLinearRegressor&
```

### Example 2 (Demo Pattern)

```cpp
  try {
    raw = DataFrame::load_csv(data_path.string(), {{"ncases", ncases}});
  } catch (const std::exception& ex) {
    std::cerr << "load_csv failed: " << ex.what() << "\n";
    return 1;
  }

  //========== schema normalization ========
  // This section gives consistent names and explicit core column dtypes.
  DataFrame frame = raw.rename_features({
      {"dteday", "date"},
      {"holiday", "is_holiday"},
      {"workingday", "is_workingday"},
      {"weathersit", "weather_condition"},
      {"hum", "humidity"},
      {"mnth", "month"},
      {"cnt", "total_count"},
      {"hr", "hour"},
      {"yr", "year"},
  });
```

**Expected output**

```text
core returned ::pluss::ai::nodej::online::OnlineLinearRegressor&
```
