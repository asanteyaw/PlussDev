# `Result::missing_rate`

## API Signature

- `Result::missing_rate() -> const Series&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::missing_rate();
out.info("missing_rate", value);
```

**Expected output**

```text
missing_rate returned const Series&
```

### Example 2 (Demo Pattern)

```cpp
      "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
  });
  // Axis::Case reduces down each column across all rows, producing one missingness rate per
  // feature; the result is a Feature-labeled Thread, so its own labels are the original feature
  // names. Those labels are surfaced both as an explicit "feature" column (so they show up in
  // the CSV, which is saved with the library's default add_cases=false) and as the summary
  // frame's own case labels.
  const Thread missing_rate_thread = missing_binary.mean(pluss::table::Axis::Case);
  const DataFrame missing_rate = DataFrame(
      {
          {"feature", missing_rate_thread.label().values()},
          {"missing_rate", missing_rate_thread},
      },
      missing_rate_thread.label().values());
  missing_rate.save_csv((out_dir / "missing_rate.csv").string());

  //========== exploratory data analysis (EDA) ========
  // This section saves descriptive statistics and correlations.
  const DataFrame numeric_block = frame.features({
      "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
      "temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count",
```

**Expected output**

```text
missing_rate returned const Series&
```
