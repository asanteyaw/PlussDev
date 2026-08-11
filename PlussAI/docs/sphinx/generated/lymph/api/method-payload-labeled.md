# `Payload::labeled`

## API Signature

- `Payload::labeled() -> const LabeledBatch&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const LabeledBatch&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::labeled();
out.info("labeled", value);
```

**Expected output**

```text
labeled returned const LabeledBatch&
```

### Example 2 (Demo Pattern)

```cpp
  //========== missingness diagnostics ========
  // This section computes missingness rates before model preprocessing.
  const DataFrame missing_binary = frame.isnull({
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
```

**Expected output**

```text
labeled returned const LabeledBatch&
```
