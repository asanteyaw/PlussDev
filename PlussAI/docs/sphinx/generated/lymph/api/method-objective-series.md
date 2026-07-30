# `objective_series`

## API Signature

- `objective_series(report: const TuneReport&) -> Series`

## Notes

Namespace-level API function.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = objective_series(/* report */);
out.info("objective_series", value);
```

**Expected output**

```text
objective_series returned Series
```
