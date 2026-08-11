# `Result::mean`

## API Signature

- `Result::mean() -> const Series&`
- `Result::mean() -> Series`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Series`.
- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::mean();
out.info("mean", value);
```

**Expected output**

```text
mean returned const Series&
```

### Example 2 (Demo Pattern)

```cpp
      "temp", "atemp", "humidity", "windspeed", "total_count",
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
```

**Expected output**

```text
mean returned const Series&
```
